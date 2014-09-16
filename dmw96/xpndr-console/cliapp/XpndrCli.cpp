#include "XpndrCli.h"
#include "MenuCommands.h"


CXpndrWifiCli::CXpndrWifiCli(CWifiTestMgr* 	pWifiTestMgr,
							 bool bIsSilent):m_pWifiTestMgr(pWifiTestMgr)

{
	for(int ind=0;ind < MAX_NUMBER_OF_CLI_COMMANDS;ind++)
	{
		m_pCmdArr[ind] = NULL;
	}

	CreateMenu(bIsSilent);

}

CXpndrWifiCli::CXpndrWifiCli(CWifiTestMgr* 	pWifiTestMgr,
							 char* fileName,
							 bool bIsSilent):m_pWifiTestMgr(pWifiTestMgr)

{
	for(int ind=0;ind < MAX_NUMBER_OF_CLI_COMMANDS;ind++)
	{
		m_pCmdArr[ind] = NULL;
	}

	CreateMenu(bIsSilent);

	RedirectInputToFile(fileName);


}


CXpndrWifiCli::CXpndrWifiCli(CWifiTestMgr*  pWifiTestMgr,
							 char* pArgsArr[],
							 int numOfStreamArgs,
							 bool bIsSilent):m_pWifiTestMgr(pWifiTestMgr)

{
	for(int ind=0;ind < MAX_NUMBER_OF_CLI_COMMANDS;ind++)
	{
		m_pCmdArr[ind] = NULL;
	}

	CreateMenu(bIsSilent);

	RedirectInputToArgStream(pArgsArr,numOfStreamArgs);

}


CXpndrWifiCli::~CXpndrWifiCli()
{
	for(int ind=0;ind < MAX_NUMBER_OF_CLI_COMMANDS;ind++)
	{
		if(m_pCmdArr[ind] != NULL)
		{
			delete m_pCmdArr[ind];
		}
		else
		{
			break;
		}
	}

	if(m_pMenu)
	{
		delete m_pMenu;
	}

}

int CXpndrWifiCli::Run()
{
	if(m_pMenu == NULL)
	{
		return -1;
	}

	ECliMenuRes res = m_pMenu->Start();

	if(res != CLIMENU_RES_OK)
	{
		return -2;
	}

	if(m_filestr.is_open())
	{
		m_filestr.close();
	}

	m_pMenu->ResetIOStreams();

	return 0;
}

int CXpndrWifiCli::RedirectInputToFile(char* fileName)
{
	if(m_pMenu == NULL ||  fileName == NULL)
	{
			return -1;
	}

	m_filestr.open (fileName);

	if(m_filestr.fail())
	{
		return -2;
	}

	m_pMenu->SetIOStreams(m_filestr,std::cout);

	return 0;


}

int CXpndrWifiCli::RedirectInputToArgStream(char* pArgsArr[],
											int numOfStreamArgs)
{
	if(m_pMenu == NULL ||  pArgsArr == NULL)
	{
			return -1;
	}

	cout<<"Redirecting to input arg strem"<<endl;

	for(int argInd = 0;argInd < numOfStreamArgs;argInd++)
	{
		m_argInputStream<<pArgsArr[argInd]<<endl;
		cout<<"Arg num "<<argInd<<" is "<<pArgsArr[argInd]<<endl;

	}

	m_pMenu->SetIOStreams(m_argInputStream,std::cout);

	return 0;
}


void CXpndrWifiCli::CreateMenu(bool   bIsSilent)
{
	ECliMenuRes res;

	EMenuDisplayMode displayMode = DISPLAYMODE_FULL;

	if(bIsSilent == true)
	{
		displayMode = DISPLAYMODE_SILENT;
	}



	//Create CLI Menu object
	m_pMenu = new CCliMenu("XpndrCLI",
						   "Main Menu",
						    INPUTMODE_BOTH,
							displayMode);


	if(m_pMenu == NULL)
		return;


	m_pTxPacketSettings = new CTxPacketSettings();

	if(m_pTxPacketSettings == NULL)
	{
		delete m_pMenu;

		m_pMenu = NULL;

		return;
	}


	m_pRxSettings = new CRxSettings();

	if(m_pRxSettings == NULL)
	{
		delete m_pMenu;

		delete m_pTxPacketSettings;

		m_pMenu = NULL;

		m_pTxPacketSettings = NULL;

		return;
	}


	res = BuildMenus();

	if(res != CLIMENU_RES_OK)
	{
		//check what to do
	}
}

ECliMenuRes CXpndrWifiCli::BuildMenus()
{

	MenuHandle curMenuHandle;
	uint32 cmdIndex = 0;
	ECliMenuRes res = CLIMENU_RES_OK;

	if(m_pMenu == NULL)
	{
		res = CLIMENU_RES_ARG_ERR;
		goto error_occured;
	}


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "PhyRateAdpt",
							  "Enable/Disable Automatic Phy rate adaptation",
							  'a');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPhyRateAdoptGet(m_pWifiTestMgr,
												  "Get",
												  "Get current settings",
												  'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPhyRateAdoptEnable(m_pWifiTestMgr,
													 "Enable",
													 "Enable Automatic Phy rate adaptation",
													 'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPhyRateAdoptDisable(m_pWifiTestMgr,
													 "Disable",
													 "Disable Automatic Phy rate adaptation",
													  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//-----------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "MaxTxPhyRate",
							  "Set Max Tx Phy Rate",
							  'b');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdMaxPhyRateGet(m_pWifiTestMgr,
												"Get",
												"Get current settings",
												'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdMaxPhyRateSet(m_pWifiTestMgr,
												"Set",
												"Set Max Tx Phy Rate",
												'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//-----------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "Freq",
							  "Set Frequency",
							  'c');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdFreqGet(m_pWifiTestMgr,
												"Get",
												"Get current settings",
												'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdFreqSet(m_pWifiTestMgr,
										"Set",
										"Set Freq",
										'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdFreqGetScanListChanFreq(m_pWifiTestMgr,
														  "GetScanFreqList",
										"Get Scan Channel Frequencies List",
										'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdFreqSetScanListChannel(m_pWifiTestMgr,
														 "SetScanListChannel",
														"Set Scan Channel",
														 'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdFreqAddScanListChannel(m_pWifiTestMgr,
														 "AddScanListChannel",
														"Add Scan Channel to list",
														 'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


//-----------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "TxPower",
							  "Set Tx Power Settings",
							  'd');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPowerMaxGet(m_pWifiTestMgr,
											  "GetMax",
											  "Get Max Tx Power in dBm",
											  'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerMaxSet(m_pWifiTestMgr,
											  "SetMax",
											  "Set Max Tx Power in dBm",
											  'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerCurGet(m_pWifiTestMgr,
											  "GetCurLevel",
											  "Get Current Tx Power Level",
											  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerCurSet(m_pWifiTestMgr,
											  "SetCurLevel",
											  "Set Current Tx Power Level [0..63]",
											  'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerCurSetAll(m_pWifiTestMgr,
												 "SetCurLevelAll",
												 "Set Current Tx Power Level "
												 "for all frequencies and rates[0..63]",
												 'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerCurLoadTable(m_pWifiTestMgr,
													"LoadTable",
													"Load Tx Power Level Table",
													'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


//----------------------------------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "PacketGen",
							  "Set internal packet generation",
							  'e');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPacketGenGet(m_pWifiTestMgr,
											   m_pTxPacketSettings,
											  "Get",
											  "Get current settings",
											  'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPacketGenSetPacketSize(m_pWifiTestMgr,
														 m_pTxPacketSettings,
														 "PacketSize",
														 "Set internal packet generation packet size [0..1500]",
														 'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPacketGenSetPacketRate(m_pWifiTestMgr,
														 m_pTxPacketSettings,
														 "PacketRate",
														 "Set packet tx rate",
														 'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPacketGenSetPacketPowerLevel(m_pWifiTestMgr,
															   m_pTxPacketSettings,
															   "PowerLevel",
															   "Set Power Level",
															   'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPacketGenSetPacketFillType(m_pWifiTestMgr,
															 m_pTxPacketSettings,
															 "FillType",
															 "Set fill type 0 - Fixed,1 - Random, 2 - Incremental",
															 'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPacketGenGenerate(m_pWifiTestMgr,
													m_pTxPacketSettings,
													"Generate",
													"Generate packets",
													'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPacketGenStop(m_pWifiTestMgr,
												m_pTxPacketSettings,
												"StopSend",
												"Stop generating packets",
												'g');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//-------------------------------------------


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "Receive",
							  "Receive Packets",
							  'f');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdReceivePacketsGet(m_pWifiTestMgr,
													m_pRxSettings,
													"Get",
													"Get current settings",
													'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdReceivePacketsSetMacFilter(m_pWifiTestMgr,
															 m_pRxSettings,
															 "Mac",
															 "Set Mac Filter",
															 'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdReceivePacketsUnSetMacFilter(m_pWifiTestMgr,
															   m_pRxSettings,
															  "MacOff",
															  "UnSet Mac Filter",
															  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdReceivePacketsSetTimeout(m_pWifiTestMgr,
														   m_pRxSettings,
														   "TimeOut",
														   "Set Receive Timeout",
														   'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdReceivePacketsStart(m_pWifiTestMgr,
													  m_pRxSettings,
													  "Start",
													  "Start Recive Packets",
													  'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdReceivePacketsStop(m_pWifiTestMgr,
													 m_pRxSettings,
													 "Stop",
													 "Stop Reciving Packets",
													 'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//----------------------------------------------------------


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "CWTransmit",
							  "Enable/Disable CW tranmission",
							  'g');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdCWTranmissionEnable(m_pWifiTestMgr,
													  "Enable",
													  "Enable CW tranmission",
													  'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdCWTranmissionDisable(m_pWifiTestMgr,
													   "Disable",
													   "Disable CW tranmission",
													   'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


//------------------------------------------------------------


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "RSSI",
							  "Get RSSI per received beacons/packets",
							  'h');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRSSIGet(m_pWifiTestMgr,
										 "Get",
										 "Get current settings",
										 'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRSSISetBeaconWndSize(m_pWifiTestMgr,
													   "SetB",
													   "Set averaging window size for RSSI per received beacons",
													   'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRSSISetDataWndSize(m_pWifiTestMgr,
													 "SetD",
													 "Set averaging window size for RSSI per received packets other then beacons",
													 'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRSSIBeacon(m_pWifiTestMgr,
											 "RSSIBeacon",
											 "Get RSSI per received beacons",
											 'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRSSIData(m_pWifiTestMgr,
										   "RSSIData",
										   "Get RSSI per received packets other then beacons",
										   'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


//---------------------------------------------------------


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "Registers",
							  "Operations on registers",
							  'i');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRegistersWrite(m_pWifiTestMgr,
												 "WriteReg",
												 "Write registers (BB,RF,General)",
												 'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRegistersRead(m_pWifiTestMgr,
												"ReadReg",
												"Read registers (BB,RF,General)",
												'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdRegistersWriteField(m_pWifiTestMgr,
													  "WriteRegField",
													  "Write field (bitmask) of register (BB,RF)",
													  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRegistersReadField(m_pWifiTestMgr,
													 "ReadRegField",
													 "Read field (bitmask) of register (BB,RF)",
													 'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdRegistersWriteBit(m_pWifiTestMgr,
													"WriteRegBit",
													"Set bit of register (BB,RF)",
													'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRegistersReadBit(m_pWifiTestMgr,
												   "ReadRegBit",
												   "Read bit of register (BB,RF)",
												   'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] =
		new CCmdRegistersSave(m_pWifiTestMgr,
		"SaveRegToFile",
		"Save all BB or RF registers "
		"to a file",
					'g');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdRegistersLoad(m_pWifiTestMgr,
												"LoadReg",
												"Load BB or RF registers from a file",
												'h');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRegistersUpload(m_pWifiTestMgr,
												  "UploadRegFile",
												  "Upload BB or RF registers file",
												  'i');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdRegistersDownload(m_pWifiTestMgr,
												    "DownloadRegFile",
												    "Download BB or RF registers file",
												    'j');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//------------------------------------------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "SetDFSChannel",
							  "Override the DFS channel",
							  'j');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDFSChannelGet(m_pWifiTestMgr,
												"Get",
												"Get current settings",
												'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDFSChannelSet(m_pWifiTestMgr,
												"Set",
												"Set current DFS Channel",
												'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


//-------------------------------------------------------------------



	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "DFSEvents",
							  "Enable/Disable DFS events",
							  'k');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDFSEventGet(m_pWifiTestMgr,
											  "Get",
											  "Get current settings",
											  'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDFSEventEnable(m_pWifiTestMgr,
												 "Enable",
												 "Enable  DFS events",
												 'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDFSEventDisable(m_pWifiTestMgr,
												  "Disable",
												  "Disable DFS events",
												  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//-----------------------------------------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "PwrSave",
							  "Set Power Save Mode",
							  'l');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPowerSaveGet(m_pWifiTestMgr,
											  "Get",
											   "Get current settings",
											   'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerSaveEnable(m_pWifiTestMgr,
												  "Enable",
												  "Enable Power Save",
												   'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPowerSaveDisable(m_pWifiTestMgr,
												  "Disable",
												  "Disable Power Save",
												  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPowerSaveSetAction(m_pWifiTestMgr,
												   "SetAction",
													"Set Power Save Action",
													'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerSaveSetAcMode(m_pWifiTestMgr,
												     "SetAcMode",
													 "Set Ac Power Save Mode",
													 'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}
#ifndef ANDROID
	m_pCmdArr[cmdIndex] = new CCmdPowerSaveTriggerFrameStart(m_pWifiTestMgr,
													 "StartTriggerFrameMech",
													 "Start Sending Trigger Frame Packets",
													 'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdPowerSaveTriggerFrameStop(m_pWifiTestMgr,
													 "StopTriggerFrameMech",
													 "Stop Sending Trigger Frame Packets",
													 'g');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}
#endif //ANDROID
/*	m_pCmdArr[cmdIndex] = new CCmdPowerSaveSetAlwaysOn(m_pWifiTestMgr,
													   "ALWAYSON",
													   "Set Power Save Mode to ALWAYSON",
													   'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPowerSaveSetDoze(m_pWifiTestMgr,
												   "DOZE",
												   "Set Power Save Mode to DOZE",
												   'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdPowerSaveSetDeepSleep(m_pWifiTestMgr,
														"DEEPSLEEP",
														"Set Power Save Mode to DEEPSLEEP",
														'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}
*/
//---------------------------------------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "Nuisance",
							  "Enable/Disable Nuisance Packet Filtering",
							  'm');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdNuisanceGet(m_pWifiTestMgr,
											  "Get",
											  "Get current settings",
											  'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdNuisanceEnable(m_pWifiTestMgr,
												 "Enable",
												 "Enable Nuisance Packet Filtering",
												 'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdNuisanceDisable(m_pWifiTestMgr,
												  "Disable",
												  "Disable Nuisance Packet Filtering",
												  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//----------------------------------------------------


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "Diversity",
							  "Set Antenna Diversity Mode",
							  'n');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDiversityGet(m_pWifiTestMgr,
											  "Get",
											  "Get current settings",
											  'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDiversityMRC(m_pWifiTestMgr,
											  "MRC",
											  "Set Antenna Diversity Mode to MRC (J2 for RF22)",
											  'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdDiversityAntennaA(m_pWifiTestMgr,
													"AntennaA",
													"Set Antenna Diversity Mode to Antenna A (J2 for RF22)",
													'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDiversityAntennaB(m_pWifiTestMgr,
													"AntennaB",
													"Set Antenna Diversity Mode to Antenna B (J1 for RF22)",
													'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//---------------------------------------------------


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "Mib",
							  "Set/Get Miboids",
							  'o');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdMib32Get(m_pWifiTestMgr,
										   "GetMib32",
										   "Read Mib (uint32)",
										   'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdMib32Set(m_pWifiTestMgr,
										   "SetMib32",
										   "Set Mib (uint32)",
										   'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdMibMacAddrGet(m_pWifiTestMgr,
												"GetMibMacAddr",
												"Read Mib (Mac Address)",
												'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdMibMacAddrSet(m_pWifiTestMgr,
												"SetMibMacAddr",
												"Set Mib (Mac Address)",
												'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdMibSsidGet(m_pWifiTestMgr,
												"GetMibSsid",
												"Read Mib (Ssid)",
												'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdMibSsidSet(m_pWifiTestMgr,
												"SetMibSsid",
												"Set Mib (Ssid)",
												'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

//---------------------------------------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "Driver",
							  "Set/Get Driver mode and state",
							  'p');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDriverModeGet(m_pWifiTestMgr,
												"Get",
												"Get Driver Working Mode",
												'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDriverModeSet(m_pWifiTestMgr,
												"Set",
												"Set Driver Working Mode",
												'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdDriverStart(m_pWifiTestMgr,
											  "Start",
											  "Start Driver",
											  'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdDriverStop(m_pWifiTestMgr,
											 "Stop",
											 "Stop Driver",
											 'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdDBGMsgGetInteger(m_pWifiTestMgr,
												"Get DBG Filter",
												"Get Current Debug messages filteiring Status",
												'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}
//g_dwDbgFilter , g_dwDbgModules


m_pCmdArr[cmdIndex] = new CCmdDBGMsgSetInteger(m_pWifiTestMgr,
												"Set DBG Filter",
												"Set Debug messages Filter (Level) and Module",
												'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}




//------------------------------------------------------------------

	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "RFTest",
							  "Special commands for RF Testing",
							  'q');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestTransmit(m_pWifiTestMgr,
												"TransmitStart",
												"Start packet generation",
												'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestTransmitStop(m_pWifiTestMgr,
													 "TransmitStop",
													 "Stop packet generation",
													 'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestReceive(m_pWifiTestMgr,
												"ReceiveStart",
												"Start packet receiving proc",
												'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestReceiveStop(m_pWifiTestMgr,
													"ReceiveStop",
													"Stop packet receiving proc",
													'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestRateMask(m_pWifiTestMgr,
												"RateMask",
												"Enable / Disable rates",
												'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestRxGainCalibration(m_pWifiTestMgr,
														   "RxGainClb",
														   "Rx Gain Calibration",
														   'f');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestTransmitEnhanced(m_pWifiTestMgr,
												"TransmitStart - Ex",
												"Start packet generation with DACOFFSET",
												'g');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdRFTestReceiveEnhanced(m_pWifiTestMgr,
												"ReceiveStart - Ex",
												"Start packet receiving proc with InitAgain",
												'h');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdRFTestMonitor(m_pWifiTestMgr,
												"Rf Monitor-Read",
												"Read Rx/Tx Monitoring Data",
												'r');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdRFTestMonitorControl(m_pWifiTestMgr,
												"Control TM Monitor",
												"Control Rx/Tx Monitoring",
												'j');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}
	


	res = m_pMenu->AddSubMenu(MENU_ROOT,
							  &curMenuHandle,
							  "SysSettings",
							  "Set/Get commands for System Setting DB",
							  'r');

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdSysSettingGetInteger(m_pWifiTestMgr,
												"Get Integer",
												"Get Integer Value",
												'a');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdSysSettingSetInteger(m_pWifiTestMgr,
												"Set Integer",
												"Set Integer Value",
												'b');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdSysSettingGetString(m_pWifiTestMgr,
												"Get String",
												"Get String Value",
												'c');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdSysSettingSetString(m_pWifiTestMgr,
												"Set String",
												"Set String Value",
												'd');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdSysSettingSyncInteger(m_pWifiTestMgr,
												 "Sync",
												 "Sync Wifi Manager with Integer Value",
												 'e');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdRfCalibration(m_pWifiTestMgr,
												"Rf Calibration",
												"Wifi Rf Calibration",
												'f');

    res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdSysSettingGetIntegerFromFile(m_pWifiTestMgr,
												"Get Int From File",
												"Get Integer Value From File",
												'g');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	m_pCmdArr[cmdIndex] = new CCmdSysSettingSetIntegerInFile(m_pWifiTestMgr,
												"Set Int In File",
												"Set Integer Value in file",
												'h');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdSysSettingGetStringFromFile(m_pWifiTestMgr,
												"Get Str From File",
												"Get String Value from file",
												'i');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdSysSettingSetStringInFile(m_pWifiTestMgr,
												"Set String In File",
												"Set String Value In File",
												'j');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	m_pCmdArr[cmdIndex] = new CCmdSysSettingGetBoardId(m_pWifiTestMgr,
												"Get Board ID",
												"Gets Board ID",
												'k');

	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}


	m_pCmdArr[cmdIndex] = new CCmdSysSettingGetBoardRfParam(m_pWifiTestMgr,
												"Get RF Settings",
												"Gets RF Boards' Settings for RX Calibration",
												'l');



	res = m_pMenu->AddCommandToSubMenu(curMenuHandle,m_pCmdArr[cmdIndex++]);

	if(res != CLIMENU_RES_OK)
	{
		goto error_occured;
	}

	//------------ END Of Menus ---------



	return CLIMENU_RES_OK;

error_occured:

	for(uint32 ind=0;ind < cmdIndex - 1;ind++)
	{
		if(m_pCmdArr[ind] != NULL)
		{
			delete m_pCmdArr[ind];

			m_pCmdArr[ind] = NULL;

		}
		else
		{
			break;
		}
	}

	if(m_pMenu)
	{
		delete m_pMenu;

		m_pMenu = NULL;
	}

	return res;

}

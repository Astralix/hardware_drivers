#include "MenuCommands.h"
#include  <vector>
#include  <fstream>
#include  <sstream>
#define RTL80211_DEBUG_MODE 5

#ifndef ANDROID
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include "Interfaces/NonCom/SysSettings/ISysSettings.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "xpndr-syssettings/include/ISysSettings.h"
#endif //ANDROID

void CCmdPhyRateAdoptGet::WifiAction()
{

	EWifiTestStatus status;

	bool rateAdoptSettings = false;

	status = m_pWifiTestMgr->GetAutoPhyRateAdoptation(&rateAdoptSettings);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting Phy Rate Adoptation settings.",(int)status);
		return;

	}

	if(rateAdoptSettings == true) {

		DisplaySuccessMsg("Phy Rate Adoptation enabled");

	}
	else {

        DisplaySuccessMsg("Phy Rate Adoptation disabled");

	}
}

void CCmdPhyRateAdoptEnable::WifiAction()
{

	EWifiTestStatus status;

    status = m_pWifiTestMgr->SetAutoPhyRateAdoptation(true);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error enabling Phy Rate Adoptation.",(int)status);
		return;

	}

	DisplaySuccessMsg("Phy Rate Adoptation enabled");

}

void CCmdPhyRateAdoptDisable::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->SetAutoPhyRateAdoptation(false);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error disabling Phy Rate Adoptation.",(int)status);
		return;

	}

	DisplaySuccessMsg("Phy Rate Adoptation disabled");

}

void CCmdMaxPhyRateGet::WifiAction()
{

	EWifiTestStatus status;

	uint32 maxRate = 0;

	status = m_pWifiTestMgr->GetMaxTxPhyRate(&maxRate);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting Max Phy Rate.",(int)status);
		return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Max Tx Rate is:\t%lu Mb/s",maxRate);

	DisplaySuccessMsg(tmpStr);

}

void CCmdMaxPhyRateSet::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new Max Phy. Rate>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	status = m_pWifiTestMgr->SetMaxTxPhyRate((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting Max Phy Rate.",(int)status);

		return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Max Tx Rate was set to:\t%d Mb/s",iInput);

	DisplaySuccessMsg(tmpStr);

}

void CCmdFreqGet::WifiAction()
{
	EWifiTestStatus status;

	uint32 freq = 0;

	status = m_pWifiTestMgr->GetTxFreq(&freq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting frequency.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Tx Freq is:\t%lu MHz",freq);

	DisplaySuccessMsg(tmpStr);


}

void CCmdFreqSet::WifiAction()
{

	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new Tx Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	status = m_pWifiTestMgr->SetTxFreq((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting frequency.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Frequency was set to:\t%d MHz",iInput);

	DisplaySuccessMsg(tmpStr);

}

void CCmdFreqGetScanListChanFreq::WifiAction()
{
	EWifiTestStatus status;

	WmScanChanFreqList scanChanFreqList;

	status = m_pWifiTestMgr->GetScanChanFreqList(&scanChanFreqList);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting scan list chan frequencies list.",(int)status);

		return;
	}

	std::string strOutput = "";

	char tmpStr[10];

	for(uint8 ind =0 ; ind < scanChanFreqList.nChanFreq; ind++) {

		sprintf(tmpStr,"%u,\t",scanChanFreqList.chanFreq[ind]);

		strOutput += tmpStr;

		if(!(ind % 10) && ind > 0)
		{
			DisplayMsg(strOutput);

			strOutput = "";
		}
	}

	DisplayMsg(strOutput);


	DisplaySuccessMsg("Scan Channel Frequencies List has been read");

}

void CCmdFreqSetScanListChannel::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new scan Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	status = m_pWifiTestMgr->SetScanChannelFreq((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting scan channel frequency.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Scan channel frequency was set to:\t%d MHz",iInput);

	DisplaySuccessMsg(tmpStr);
}

void CCmdFreqAddScanListChannel::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new scan Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	status = m_pWifiTestMgr->AddChannelToScanFreqList((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error adding scan channel frequency.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Scan channel frequency was added:\t%d MHz",iInput);

	DisplaySuccessMsg(tmpStr);

}



void CCmdPowerMaxGet::WifiAction()
{

	EWifiTestStatus status;

    uint32 txPower = 0;

	status = m_pWifiTestMgr->GetTxPower(&txPower);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting Max Tx Power.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Max Tx Power is:\t%lu dBm",txPower);

	DisplaySuccessMsg(tmpStr);

}



void CCmdPowerMaxSet::WifiAction()
{

	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new Max Tx Power in dBm>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < -20 || iInput > 30) {

		DisplayErrorMsg("Wrong Input! Input should be [-20:30] dBm.");

		return;
	}

	status = m_pWifiTestMgr->SetTxPower((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting Max Tx Power.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Max Tx Power was set to:\t%lu dBm",(uint32)iInput);

	DisplaySuccessMsg(tmpStr);

}

void CCmdPowerCurGet::WifiAction()
{
	EWifiTestStatus status;

	int iInput;


	std::string strPromt = "Please enter input type [0 - By Index,1 - By Values]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 1)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - By Index,1 - By Values].");

		return;

	}

	uint8 txPower = 0;

	if(iInput == 0)
	{

		strPromt = "Please enter frequency index>";

		bool res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0 || iInput > WM_POWERTABLE_MAX_FREQ_NUM) {

			DisplayErrorMsg("Wrong Input! Frequency index should be in a range [0..%d]",WM_POWERTABLE_MAX_FREQ_NUM-1);

			return;
		}

		uint8 iFreqInd = (uint8)iInput;

		strPromt = "Please enter rate index>";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0 || iInput > WM_POWERTABLE_MAX_RATE_NUM) {

			DisplayErrorMsg("Wrong Input! Rate index should be in a range [0..%d]",WM_POWERTABLE_MAX_RATE_NUM-1);

			return;
		}

        status = m_pWifiTestMgr->GetTxPowerLevelByIndex(iFreqInd,
														(uint8)iInput,
														&txPower);

	}
	else
	{

		strPromt = "Please enter frequency>";

		bool res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0) {

			DisplayErrorMsg("Wrong Input! Frequency should be larger then 0");

			return;
		}

		uint16 iFreq = (uint16)iInput;

		strPromt = "Please enter rate>";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0) {

			DisplayErrorMsg("Wrong Input! Rate should be larger then 0");

			return;
		}

		status = m_pWifiTestMgr->GetTxPowerLevel(iFreq,
												 (uint8)iInput,
												 &txPower);

	}

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error Getting TX Power Level.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Tx Power Level is:\t%lu",(uint32)txPower);

	DisplaySuccessMsg(tmpStr);

}

void CCmdPowerCurSet::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter input type [0 - By Index,1 - By Values]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 1)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - By Index,1 - By Values].");

		return;

	}

	uint8 iInputType = (uint8)iInput;

	uint16 iFreq;

	uint8 iRate;


	if(iInputType == 0)
	{

		strPromt = "Please enter frequency index>";

		bool res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0 || iInput > WM_POWERTABLE_MAX_FREQ_NUM) {

			DisplayErrorMsg("Wrong Input! Frequency index should be in a range [0..%d]",WM_POWERTABLE_MAX_FREQ_NUM-1);

			return;
		}

		iFreq = (uint16)iInput;

		strPromt = "Please enter rate index>";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0 || iInput > WM_POWERTABLE_MAX_RATE_NUM) {

			DisplayErrorMsg("Wrong Input! Rate index should be in a range [0..%d]",WM_POWERTABLE_MAX_RATE_NUM-1);

			return;
		}

		iRate = (uint8)iInput;

	}
	else
	{

		strPromt = "Please enter frequency>";

		bool res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0) {

			DisplayErrorMsg("Wrong Input! Frequency should be larger then 0");

			return;
		}

		iFreq = (uint16)iInput;

		strPromt = "Please enter rate>";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0) {

			DisplayErrorMsg("Wrong Input! Rate should be larger then 0");

			return;
		}

		iRate = (uint8)iInput;
	}

	strPromt = "Please enter new power level>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 63) {

		DisplayErrorMsg("Wrong Input! Power Level should be in a range [0..63]");

		return;
	}


	uint8 txPower = (uint8)iInput;

	if(iInputType == 0) {

		status = m_pWifiTestMgr->SetTxPowerLevelByIndex((uint8)iFreq,
														iRate,
														txPower);
	}
	else
	{
		status = m_pWifiTestMgr->SetTxPowerLevel(iFreq,
												 iRate,
												 txPower);
	}

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error Setting TX Power Level.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Tx Power Level was set to is:\t%lu",(uint32)txPower);

	DisplaySuccessMsg(tmpStr);

}



void CCmdPowerCurSetAll::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter Tx Power level [0..63]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 63) {

		DisplayErrorMsg("Wrong Input! Input should be [0:63]");

		return;
	}

	status = m_pWifiTestMgr->SetTxPowerLevelAll((uint8)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting  Tx Power Level.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Tx Power Level was set to:\t%lu",(uint32)iInput);

	DisplaySuccessMsg(tmpStr);

}


void CCmdPowerCurLoadTable::WifiAction()
{

	EWifiTestStatus status;


	std::string strPromt = "Please enter Power Level Table file name>";

	std::string inpStr;

	bool res = GetInputFromUser_String(inpStr,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	WmPowerLevelTable txPowerLevelTable;

	status = m_pWifiTestMgr->GetTxPowerLevelTable(&txPowerLevelTable);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting  Power Level Table.",(int)status);

		return;
	}


	ifstream fs;

	fs.open(inpStr.c_str());

	if(fs.fail())
	{
		DisplayErrorMsg("Cannot open file");

		return;
	}

	int iRes;

	uint32 errLineNum = 0;

	iRes = ParsePowerTableFile(fs,
							   &txPowerLevelTable,
							   &errLineNum);

	fs.close();

	if(iRes < 0)
	{
		DisplayErrorMsg("Error while parsing input file");

		return;
	}


	status = m_pWifiTestMgr->SetTxPowerLevelTable(&txPowerLevelTable);

    if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting  Power Level Table.",(int)status);

		return;
	}


	DisplaySuccessMsg("Power table was loaded  successfully");


}

int CCmdPowerCurLoadTable::ConvertField(const std::string& field,uint32* pCurVal)
{

	stringstream ss(field);

	ss >> *pCurVal;

    if(!std::cin)
	{
	  return -1;
	}

	return 0;

}

int CCmdPowerCurLoadTable::ParseLine(vector<uint32>& valVector,
									 const std::string& field)
{
	stringstream recStream(field);

	string tmpField;

	uint32 curVal;

	int iRes;

	while(getline(recStream,tmpField,','))
	{
		iRes = ConvertField(tmpField,&curVal);

		if(iRes < 0)
		{
			return -1;
		}

		valVector.push_back(curVal);
	}

	return 0;

}


int CCmdPowerCurLoadTable::UpdatePowerLevelTable(vector<uint32>& valVector,
												 WmPowerLevelTable* pPowerLevelTable)
{
	vector<uint32>::const_iterator it = valVector.begin();

	uint32 curInd = 0;
	uint8 curFreqInd = 0;
	uint8 curRateInd = 0;
	uint8 curPowerLevel = 0;

	while (it != valVector.end())
	{
		//cout<<"ParamNum "<<curInd<<" is "<<(uint32)(*it)<<endl;

		if( !(curInd % 3) )
		{
			curFreqInd = (uint8)(*it);

			if( curFreqInd > WM_POWERTABLE_MAX_FREQ_NUM )
			{
				return -1;
			}
		}
		else if( !( (curInd + 2) % 3) )
		{
			curRateInd = (uint8)(*it);

			if( curRateInd > WM_POWERTABLE_MAX_RATE_NUM )
			{
				return -1;
			}

		}
		else
		{
			curPowerLevel = (uint8)(*it);

			if( curPowerLevel > 63 )
			{
				return -1;
			}

			(pPowerLevelTable->pt)[curFreqInd][curRateInd] = curPowerLevel;
		}

		curInd++;

		it++;
	}

	return 0;


}

int CCmdPowerCurLoadTable::ParsePowerTableFile(ifstream& fs,
											   WmPowerLevelTable* pPowerLevelTable,
											   uint32* pErrLineNum)
{

	*pErrLineNum = 0;

	if(fs == 0)
	{
		return -1;
	}

	std::string sLine;

	uint32 lineCounter = 0;

	int iRes;

	vector<uint32> valVector;

	//build vector of numbers

	while(!fs.eof())
	{

		getline(fs,sLine);

		lineCounter++;

		if(sLine == "")
		{
			continue;
		}

		iRes = ParseLine(valVector,sLine);

		if(iRes < 0)
		{
			*pErrLineNum = lineCounter;

			return -1;
		}

	}

    iRes = UpdatePowerLevelTable(valVector,pPowerLevelTable);

	if(iRes < 0)
	{
		*pErrLineNum = lineCounter;

		return -1;
	}

	return 0;

}


void CCmdPacketGenGet::WifiAction()
{
	DisplaySuccessMsg("");

	m_pTxPacketSettings->DisplayCurrentPacketSettings();

}

void CCmdPacketGenSetPacketSize::WifiAction()
{
	std::string strPromt = "Please enter new packet size [0..1500]>";

	int iInput;

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 1500)
	 {

		DisplayErrorMsg("Wrong Input! Packet size should be [0..1500].");

		return;


	}

	m_pTxPacketSettings->packetSize = iInput;

	DisplaySuccessMsg("");

	m_pTxPacketSettings->DisplayCurrentPacketSettings();


}

void CCmdPacketGenSetPacketRate::WifiAction()
{


	std::string strPromt = strPromt = "Please enter tx data rate (for long preamble (rates 2,5,11) add 128 to the rate value,for 11n use 64 + MCS Index[0-7])>";

	int iInput;

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput == 1 || iInput == 2  || iInput == (0x80 + 2)  ||
	   iInput == 55 || iInput == (0x80 + 55) || iInput == 6 ||
	   iInput == 9 || iInput == 11 || iInput == (0x80 + 11) ||
	   iInput == 12 || iInput == 18 || iInput == 24  || iInput == 36 ||
	   iInput == 48 || iInput == 54 || ((iInput >= 0x40) && (iInput <= 0x47)) )
	{
		m_pTxPacketSettings->phyRate = iInput;

		DisplaySuccessMsg("");

		m_pTxPacketSettings->DisplayCurrentPacketSettings();
	}

	else
	{

		DisplayErrorMsg("Wrong Input! Invalid data rate. Valid Input is [1,2,(130),55 (183),6,9,11 (139),12,18,24,36,48,54,(64-71)].");

		return;

	}

}

void CCmdPacketGenSetPacketPowerLevel::WifiAction()
{
	std::string strPromt = "Please enter new power level [0..63]>";

	int iInput;

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 63)
	 {

		DisplayErrorMsg("Wrong Input! Input should be [0:63].");

		return;

	}

	m_pTxPacketSettings->txPowerLevel = iInput;

	DisplaySuccessMsg("");

	m_pTxPacketSettings->DisplayCurrentPacketSettings();

}

void CCmdPacketGenSetPacketFillType::WifiAction()
{
	std::string strPromt = "Please enter packet payload type [0 - Fixed,1 - Random, 2 - Incremental]>";

	int iInput;

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 2)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - Fixed,1 - Random, 2 - Incremental].");

		return;

	}

	m_pTxPacketSettings->payloadType = iInput;

	DisplaySuccessMsg("");

	m_pTxPacketSettings->DisplayCurrentPacketSettings();

}

void CCmdPacketGenGenerate::WifiAction()
{


	EWifiTestStatus status;

	CWifiTestMgrPacket txPacketDefs;

	int iInput;

	std::string strPromt = "Please enter number of bursts>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0..2^32]");

		return;
	}

	m_pTxPacketSettings->numTransmits = iInput;

	txPacketDefs.sourceAddress[0]  = 0x66;
	txPacketDefs.sourceAddress[1]  = 0x66;
	txPacketDefs.sourceAddress[2]  = 0x66;
	txPacketDefs.sourceAddress[3]  = 0x77;
	txPacketDefs.sourceAddress[4]  = 0x77;
	txPacketDefs.sourceAddress[5]  = 0x77;

	txPacketDefs.destAddress[0] = 0x88;
	txPacketDefs.destAddress[1] = 0x88;
	txPacketDefs.destAddress[2] = 0x88;
	txPacketDefs.destAddress[3] = 0x99;
	txPacketDefs.destAddress[4] = 0x99;
	txPacketDefs.destAddress[5] = 0x99;

	txPacketDefs.bssId[0] = 0x44;
	txPacketDefs.bssId[1] = 0x44;
	txPacketDefs.bssId[2] = 0x44;
	txPacketDefs.bssId[3] = 0x55;
	txPacketDefs.bssId[4] = 0x55;
	txPacketDefs.bssId[5] = 0x55;

	txPacketDefs.broadcast=true;
	txPacketDefs.txRetryCount = 15;
	txPacketDefs.tid = 0;
	txPacketDefs.aifs=1;
	txPacketDefs.eCWmin=0;
	txPacketDefs.eCWmax=0;

	txPacketDefs.payloadType = m_pTxPacketSettings->payloadType;
	txPacketDefs.packetSize =  m_pTxPacketSettings->packetSize;

	status = m_pWifiTestMgr->SetTxPacketParams(m_pTxPacketSettings->numTransmits,
											   m_pTxPacketSettings->phyRate,
											   m_pTxPacketSettings->txPowerLevel	,
											   &txPacketDefs);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting packet prototype.",(int)status);

		return;
	}

    status = m_pWifiTestMgr->GeneratePackets(m_pTxPacketSettings->numTransmits);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error generating packets.",(int)status);

		return;
	}

	DisplaySuccessMsg("Starting transmit with following settings:");

	char tmpStr[100];

	sprintf(tmpStr,"NumOfTrasmits\t%lu",(uint32)(m_pTxPacketSettings->numTransmits));


	DisplayMsg(tmpStr);

	m_pTxPacketSettings->DisplayCurrentPacketSettings();

}

void CCmdPacketGenStop::WifiAction()
{

	EWifiTestStatus status;

	status = m_pWifiTestMgr->GeneratePackets(0);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error stopping packet generation.",(int)status);

		return;
	}

	DisplaySuccessMsg("Stopped packet transmition");
}

void CCmdReceivePacketsGet::WifiAction()
{

    DisplaySuccessMsg("");

	m_pRxSettings->DisplayRxSettings();

}

void CCmdReceivePacketsSetMacFilter::WifiAction()
{
		std::string strPromt = "Please enter Mac Address for filtering [xx:xx:xx:xx:xx:xx] without brackets and spaces";

		uint8 pMacFilter[6];

		bool res = GetInputFromUser_MacAddr(pMacFilter,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		for(int addrByte = 0;addrByte < 6;addrByte++) {

			(m_pRxSettings->MacFilter.Address)[addrByte] = pMacFilter[addrByte];

		}

		m_pRxSettings->isMacFilterSet = true;


		DisplaySuccessMsg("Mac filter was set.");
}

void CCmdReceivePacketsUnSetMacFilter::WifiAction()
{
		m_pRxSettings->isMacFilterSet = false;

		DisplaySuccessMsg("Mac filter was removed.");


}

void CCmdReceivePacketsSetTimeout::WifiAction()
{
	int iInput;

	std::string strPromt = "Please enter receive timeout in seconds>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


		if(iInput < 0) {

			DisplayErrorMsg("Wrong Input! Input should be large or equal to 0");

			return;

		}

		m_pRxSettings->iTimeout = iInput;

		DisplaySuccessMsg("Receive timeout was set.");
}

void CCmdReceivePacketsStart::WifiAction()
{
	EWifiTestStatus status;

	if(m_pRxSettings->isMacFilterSet)
	{
			status =  m_pWifiTestMgr->StartReceive(&(m_pRxSettings->MacFilter),m_pRxSettings->iTimeout);
	}
	else
	{
			status =  m_pWifiTestMgr->StartReceive(NULL,m_pRxSettings->iTimeout);
	}
	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting receive proc.",(int)status);

		return;

	}

	DisplaySuccessMsg("Receive proc has started.");
}

void CCmdReceivePacketsStop::WifiAction()
{
	EWifiTestStatus status;

	CWifiTestMgrRecvResults recvRes;

	status = m_pWifiTestMgr->StopReceive(&recvRes);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting received packed stats.",(int)status);

		return;

	}

	DisplaySuccessMsg("");

	CRxSettings::DisplayRxResults(&recvRes);
}

void CCmdCWTranmissionEnable::WifiAction()
{
	EWifiTestStatus status;

    int iInput;

	std::string strPromt = "Please enter new gain>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0 || iInput > 63) {

			DisplayErrorMsg("Wrong Input! Input should be [0:63]");

			return;

	}

	uint8 iCWGain = (uint8)iInput;
	uint32 reqFreq = 0;

	strPromt = "Please enter Tx Freq>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	reqFreq = iInput;


	CTxPacketSettings m_TxPacketSettings;

    uint32 curFreq = 0;

	status = m_pWifiTestMgr->GetTxFreq(&curFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting current frequency.",(int)status);

		return;

	}

	uint32 curMode = 0;

	status = m_pWifiTestMgr->GetDriverMode(&curMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting driver mode.",(int)status);

		return;

	}

	status = m_pWifiTestMgr->StopDriver();

	if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error stopping driver.",(int)status);

			return;

	}



	if(curMode != RTL80211_DEBUG_MODE)
	{
		status = m_pWifiTestMgr->SetDriverMode(RTL80211_DEBUG_MODE);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting driver mode.",(int)status);

			return;

		}
	}

	status = m_pWifiTestMgr->SetTxFreq((uint32)reqFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting frequency.",(int)status);

		return;
	}

	status = m_pWifiTestMgr->StartDriver();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting driver.",(int)status);

		return;

	}

	status = m_pWifiTestMgr->GenerateTxTone(iCWGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error generating CW.",(int)status);

		return;
	}


	DisplaySuccessMsg("CW generation enabled");

}

void CCmdCWTranmissionDisable::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdRSSIGet::WifiAction()
{
	EWifiTestStatus status;

    uint32 beaconWndSize = 0,dataWndSize = 0;

	status = m_pWifiTestMgr->GetAvgRSSIWindowSize(&beaconWndSize);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting RSSI window size.",(int)status);

		return;
	}

	status = m_pWifiTestMgr->GetAvgRSSIDataWindowSize(&dataWndSize);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting RSSI window size.",(int)status);

		return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Window size for rssi of beacon and probe requests packets is:\t%lu",beaconWndSize);

	DisplaySuccessMsg(tmpStr);

	sprintf(tmpStr,"Window size for rssi of data packets is:\t%lu",dataWndSize);

	DisplayMsg(tmpStr);

}

void CCmdRSSISetBeaconWndSize::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new window size used to calculate RSSI of received beacon and probe requests>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 1 || iInput > 100) {

		DisplayErrorMsg("Wrong Input! Input should be [1:100]");

		return;

	}


	status = m_pWifiTestMgr->SetAvgRSSIWindowSize((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting window size for beacon rssi.",(int)status);

		return;

	}


	char tmpStr[100];

	sprintf(tmpStr,"Window size for beacon and probe requests rssi was set to:\t%lu",(uint32)iInput);

	DisplaySuccessMsg(tmpStr);

}

void CCmdRSSISetDataWndSize::WifiAction()
{

	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new window size used to calculate RSSI of received data packets>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 1 || iInput > 100) {

		DisplayErrorMsg("Wrong Input! Input should be [1:100]");

		return;

	}


	status = m_pWifiTestMgr->SetAvgRSSIDataWindowSize((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting window size for data packets rssi.",(int)status);

		return;

	}


	char tmpStr[100];

	sprintf(tmpStr,"Window size for data packets rssi was set to:\t%lu",(uint32)iInput);

	DisplaySuccessMsg(tmpStr);

}

void CCmdRSSIData::WifiAction()
{
	EWifiTestStatus status;

	uint32 rssi = 0;

	status = m_pWifiTestMgr->GetAvgRSSIData(&rssi);

	if(status != WIFITEST_OK) {

        DisplayErrorMsg("Error getting RSSI for data packets.",(int)status);

		return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"RSSI for data packets is:\t%lu",rssi);

	DisplaySuccessMsg(tmpStr);

}

void CCmdRSSIBeacon::WifiAction()
{
	EWifiTestStatus status;

    uint32 rssi = 0;

	status = m_pWifiTestMgr->GetAvgRSSI(&rssi);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting RSSI for beacons and probe request.",(int)status);

		return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"RSSI for beacons and probe request packets is:\t%lu",rssi);

	DisplaySuccessMsg(tmpStr);

}

void CCmdRegistersWrite::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter register type [0 - BaseBand, 1 - RF, 2 - General, 3 - Indirect]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 3)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - BaseBand, 1 - RF, 2 - General, 3 - Indirect].");

		return;

	}

	uint8 iRegType = (uint8)iInput;


	strPromt = "Please enter register address (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}


	uint32 iRegAddr = (uint32)iInput;

	if (iRegType == 1 && iRegAddr > 256) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

		return;

	}


	strPromt = "Please enter register value (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	char tmpStr[100];

	if(iRegType == 2)
	{
        status = m_pWifiTestMgr->WriteRegister(iRegAddr,iInput);

		sprintf(tmpStr,"General Register:\t%lu was set to:\t%lu",iRegAddr,(uint32)iInput);
	}
	else
	{
		if (iInput > 256) {

			DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

			return;

		}

		ERegisterType regType = REGTYPE_BB;

		if(iRegType == 1)
		{
			regType = REGTYPE_RF;


		}
		else if(iRegType == 3)
		{
			regType = REGTYPE_INDIRECT;

		}

		status = m_pWifiTestMgr->WriteBBRegister(regType,iRegAddr,(uint8)iInput);

		if(regType == REGTYPE_BB)
		{
			sprintf(tmpStr,"BaseBand Register:\t%lu was set to:\t%lu",iRegAddr,(uint32)iInput);
		}
		else if(regType == REGTYPE_RF)
		{
			sprintf(tmpStr,"RF Register:\t%lu was set to:\t%lu",iRegAddr,(uint32)iInput);
		}
		else
		{
			sprintf(tmpStr,"Indirect Register:\t%lu was set to:\t%lu",iRegAddr,(uint32)iInput);
		}
	}


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return;
	}


	DisplaySuccessMsg(tmpStr);

}

void CCmdRegistersRead::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter register type [0 - BaseBand, 1 - RF, 2 - General, 3 - Indirect]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 3)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - BaseBand, 1 - RF, 2 - General, 3 - Indirect].");

		return;

	}

	uint8 iRegType = (uint8)iInput;

	strPromt = "Please enter register address (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	uint32 iRegAddr = (uint32)iInput;

    char tmpStr[100];

	if(iRegType == 2)
	{
		uint32 iRegValue = 0;

		status = m_pWifiTestMgr->ReadRegister(iRegAddr,&iRegValue);

		sprintf(tmpStr,"General Register:\t%lu value is:\t%lu",iRegAddr,iRegValue);
	}
	else
	{

		if (iRegAddr > 256) {

			DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

			return;
		}

		ERegisterType regType = REGTYPE_BB;

		if(iRegType == 1)
		{
			regType = REGTYPE_RF;

		}
		else if(iRegType == 3)
		{
			regType = REGTYPE_INDIRECT;

		}


		uint8 iRegByteValue;

		status = m_pWifiTestMgr->ReadBBRegister(regType,iRegAddr,&iRegByteValue);

		if(regType == REGTYPE_BB)
		{
			sprintf(tmpStr,"BaseBand Register:\t%lu value is:\t%lu",iRegAddr,(uint32)iRegByteValue);
		}
		else if(regType == REGTYPE_RF) 
		{
			sprintf(tmpStr,"RF Register:\t%lu value is:\t%lu",iRegAddr,(uint32)iRegByteValue);
		}
		else
		{
			sprintf(tmpStr,"Indirect Register:\t%lu value is:\t%lu",iRegAddr,(uint32)iRegByteValue);
		}


	}


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error reading register.",(int)status);

		return;

	}


    DisplaySuccessMsg(tmpStr);

}

void CCmdRegistersReadField::WifiAction()
{
	EWifiTestStatus status;

	int iInput;
	uint32 hInput;

	std::string strPromt = "Please enter register type [0 - BaseBand,1 - RF, 3 - Indirect]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if((iInput < 0 || iInput > 3) || (iInput==2))
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - BaseBand,1 - RF, 3 - Indirect].");

		return;

	}

	ERegisterType regType = REGTYPE_BB;

	if(iInput == 1)
	{
		regType = REGTYPE_RF;

	}
	if(iInput == 3)
	{
		regType = REGTYPE_INDIRECT;

	}
	strPromt = "Please enter register address (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	uint32 iRegAddr = (uint32)iInput;

	if (iRegAddr > 256 && regType != REGTYPE_BB) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

		return;

	}

	strPromt = "Please enter field mask (hex)>";

	res = GetInputFromUser_HexInt(&hInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(hInput > 256) 
	{

		DisplayErrorMsg("Wrong Input! Input should be in [0x00..0xFF]");

		return;

	}

	uint8 fieldMask = (uint8)hInput;

    char tmpStr[100];

	uint8 iRegByteValue;

	status = m_pWifiTestMgr->ReadBBRegisterField(regType,iRegAddr,(uint8)fieldMask,&iRegByteValue);

	if(regType == REGTYPE_BB)
	{
		sprintf(tmpStr,"BaseBand Register:\t%lu masked value is:\t%lu",iRegAddr,(uint32)iRegByteValue);
	}
	else if(regType == REGTYPE_RF)
	{
		sprintf(tmpStr,"RF Register:\t%lu masked value is:\t%lu",iRegAddr,(uint32)iRegByteValue);
	}
	else
	{
		sprintf(tmpStr,"Indirect Register:\t%lu masked value is:\t%lu",iRegAddr,(uint32)iRegByteValue);
	}



	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error reading register.",(int)status);

		return;

	}


    DisplaySuccessMsg(tmpStr);

}

void CCmdRegistersReadBit::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter register type [0 - BaseBand,1 - RF, 3 - Indirect]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if((iInput < 0 || iInput > 3) || (iInput==2))
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - BaseBand,1 - RF. 3 - Indirect].");

		return;

	}

	ERegisterType regType = REGTYPE_BB;

	if(iInput == 1)
	{
		regType = REGTYPE_RF;

	}

	if(iInput == 3)
	{
		regType = REGTYPE_INDIRECT;

	}
	strPromt = "Please enter register address (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	uint32 iRegAddr = (uint32)iInput;

	if (iRegAddr > 256 && regType != REGTYPE_BB) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

		return;

	}

	strPromt = "Please enter bit index [0..7]>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0 || iInput > 7) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..7]");

		return;

	}

	uint8 bitIndex = (uint8)iInput;

    char tmpStr[100];

	uint8 iRegByteValue;

	status = m_pWifiTestMgr->ReadBBRegisterBit(regType,iRegAddr,bitIndex,&iRegByteValue);

	if(regType == REGTYPE_BB)
	{
		sprintf(tmpStr,"BaseBand Register:\t%lu bit %u value is:\t%lu",iRegAddr,bitIndex,(uint32)iRegByteValue);
	}
	else if(regType == REGTYPE_RF)
	{
		sprintf(tmpStr,"RF Register:\t%lu bit %u value is:\t%lu",iRegAddr,bitIndex,(uint32)iRegByteValue);
	}
	else
	{
		sprintf(tmpStr,"Indirect Register:\t%lu bit %u value is:\t%lu",iRegAddr,bitIndex,(uint32)iRegByteValue);
	}



	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error reading register.",(int)status);

		return;

	}


    DisplaySuccessMsg(tmpStr);

}

void CCmdRegistersWriteField::WifiAction()
{
	EWifiTestStatus status;

	int iInput;
	uint32 hInput;


	std::string strPromt = "Please enter register type [0 - BaseBand,1 - RF, 3 - Indirect]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if((iInput < 0 || iInput > 3) || (iInput==2))
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - BaseBand,1 - RF, 3 - Indirect].");

		return;

	}

	ERegisterType regType = REGTYPE_BB;

	if(iInput == 1)
	{
		regType = REGTYPE_RF;

	}
	if(iInput == 3)
	{
		regType = REGTYPE_INDIRECT;

	}

	strPromt = "Please enter register address (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	uint32 iRegAddr = (uint32)iInput;

	if (iRegAddr > 256 && regType != REGTYPE_BB) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

		return;

	}

	strPromt = "Please enter field mask (hex)>";

	res = GetInputFromUser_HexInt(&hInput,strPromt); 

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if( hInput > 255) {

		DisplayErrorMsg("Wrong Input! Input should be in [0x00..0xFF]");

		return;

	}

	uint8 fieldMask = (uint8)hInput;

	strPromt = "Please enter register value (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}


	char tmpStr[100];

	uint8 iRegByteValue = (uint8)iInput;

	status = m_pWifiTestMgr->WriteBBRegisterField(regType,iRegAddr,(uint8)fieldMask,iRegByteValue);

	if(regType == REGTYPE_BB)
	{
		sprintf(tmpStr,"BaseBand Register:\t%lu masked was set to :\t%lu",iRegAddr,(uint32)iRegByteValue);
	}
	else if(regType == REGTYPE_RF)
	{
		sprintf(tmpStr,"RF Register:\t%lu masked value was set to:\t%lu",iRegAddr,(uint32)iRegByteValue);
	}
	else
	{
		sprintf(tmpStr,"Indirect Register:\t%lu masked value was set to:\t%lu",iRegAddr,(uint32)iRegByteValue);
	}



	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return;

	}


    DisplaySuccessMsg(tmpStr);

}


void CCmdRegistersWriteBit::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter register type [0 - BaseBand,1 - RF, 3 - Indirect]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if((iInput < 0 || iInput > 3) || (iInput==2))
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - BaseBand,1 - RF, 3 - Indirect].");

		return;

	}

	ERegisterType regType = REGTYPE_BB;

	if(iInput == 1)
	{
		regType = REGTYPE_RF;

	}
	if(iInput == 3)
	{
		regType = REGTYPE_INDIRECT;

	}

	strPromt = "Please enter register address (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	uint32 iRegAddr = (uint32)iInput;

	if (iRegAddr > 256 && regType != REGTYPE_BB) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

		return;

	}

	strPromt = "Please enter bit index [0..7]>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0 || iInput > 7) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..7]");

		return;

	}

	uint8 bitIndex = (uint8)iInput;

	strPromt = "Please enter bit value>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput != 0 && iInput != 1) {

		DisplayErrorMsg("Wrong Input! Input should be [0,1]");

		return;

	}


	char tmpStr[100];

	uint8 iRegByteValue = (uint8)iInput;

	status = m_pWifiTestMgr->WriteBBRegisterBit(regType,iRegAddr,bitIndex,iRegByteValue);

	if(regType == REGTYPE_BB)
	{
		sprintf(tmpStr,"BaseBand Register:\t%lu bit %u value was set to:\t%lu",iRegAddr,bitIndex,(uint32)iRegByteValue);
	}
	else if(regType == REGTYPE_RF)
	{
		sprintf(tmpStr,"RF Register:\t%lu bit %u value was set to:\t%lu",iRegAddr,bitIndex,(uint32)iRegByteValue);
	}
	else
	{
		sprintf(tmpStr,"Indirect Register:\t%lu bit %u value was set to:\t%lu",iRegAddr,bitIndex,(uint32)iRegByteValue);
	}



	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return;

	}


    DisplaySuccessMsg(tmpStr);

}


void CCmdRegistersSave::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdRegistersLoad::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdRegistersUpload::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdRegistersDownload::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdDFSChannelGet::WifiAction()
{
	EWifiTestStatus status;

	uint32 channel = 0;

	status = m_pWifiTestMgr->GetChannel(&channel);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting channel.",(int)status);

		return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Current channel is:\t%lu",channel);

	DisplaySuccessMsg(tmpStr);

}

void CCmdDFSChannelSet::WifiAction()
{

	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new channel.>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	status = m_pWifiTestMgr->SetChannel((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting channel.",(int)status);

        return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Channel was successfully set to:\t%lu",(uint32)iInput);

	DisplaySuccessMsg(tmpStr);

}

void CCmdDFSEventGet::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdDFSEventEnable::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdDFSEventDisable::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdPowerSaveGet::WifiAction()
{
	EWifiTestStatus status;

	bool psEnabled = false;

	status = m_pWifiTestMgr->GetPwrSave(&psEnabled);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting Power Save settings.",(int)status);
		return;

	}

	EWmPsAction pwrSaveAction;

	status = m_pWifiTestMgr->GetPwrSaveAction(&pwrSaveAction);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting Power Save Action.",(int)status);
		return;

	}

	uint32 pPsMode;;

	status = m_pWifiTestMgr->GetPsMode( &pPsMode );

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting AC 0 Power Save mode.",(int)status);
		return;

	}

	if(psEnabled == true) {

		DisplaySuccessMsg("Power Save\tenabled");

	}
	else {

		DisplaySuccessMsg("Power Save\tdisabled");

	}

	switch(pwrSaveAction) {
		case WM_POWERSAVEACTION_ALWAYSON:
			DisplayMsg("Power Save Action is:\tALWAYSON");
			break;
		case WM_POWERSAVEACTION_DOZE:
			DisplayMsg("Power Save Action is:\tDOZE");
			break;
		case WM_POWERSAVEACTION_DEEPSLEEP:
			DisplayMsg("Power Save Action is:\tDEEPSLEEP");
			break;
		default:
			DisplayErrorMsg("Power Save Action is:\tillegal");
			break;

	}

	char tmpStr[100];

	if(pPsMode){

		DisplaySuccessMsg("U-APSD mode is allowed");
	} else{
		DisplaySuccessMsg("Legacy mode is allowed");
	}
}

void CCmdPowerSaveEnable::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->SetPwrSave(true);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error enabling power save.",(int)status);

		return;

	}

	DisplaySuccessMsg("Power save is enabled");

}

void CCmdPowerSaveDisable::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->SetPwrSave(false);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error disabling power save.",(int)status);

		return;

	}

	DisplaySuccessMsg("Power save is disabled");

}

void CCmdPowerSaveSetAction::WifiAction()
{
	std::string strPromt = "Please enter Power Save Action [0 - ALLWAYSON,1 - DOZE, 2 - DEEP SLEEP]>";

	EWifiTestStatus status;

	int iInput;

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 2)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0 - ALLWAYSON,1 - DOZE, 2 - DEEP SLEEP].");

		return;

	}

	EWmPsAction pwrSaveAction;

	switch(iInput) {
		case 0:
			pwrSaveAction = WM_POWERSAVEACTION_ALWAYSON;
			strPromt = "Power Save Action set to:\tALWAYSON";
			break;
		case 1:
			pwrSaveAction = WM_POWERSAVEACTION_DOZE;
			strPromt = "Power Save Action set to:\tDOZE";
			break;
		case 2:
			pwrSaveAction = WM_POWERSAVEACTION_DEEPSLEEP;
			strPromt = "Power Save Action set to:\tDEEPSLEEP";
			break;
	}

	status = m_pWifiTestMgr->SetPwrSaveAction(pwrSaveAction);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting Power Save Action.",(int)status);

		return;

	}

	DisplaySuccessMsg(strPromt);
}


void CCmdPowerSaveSetAcMode::WifiAction()
{
	EWifiTestStatus status;

	std::string strPromt = "Choose Power save mode: 0 - U-APSD; 1 - Legacy";

	char tmpStr[100];

	int 	iInput;
	uint32 	iCurrUapsdMode;

	status = m_pWifiTestMgr->GetPsMode( &iCurrUapsdMode);

	sprintf(tmpStr, "Take care, change will affect ONLY after re-association");
	DisplayMsg(tmpStr);

	if(iCurrUapsdMode){
		sprintf(tmpStr, "Current power save mode is: UAPSD");
		DisplayMsg(tmpStr);
	} else{
		sprintf(tmpStr, "Current power save mode is: Legacy");
		DisplayMsg(tmpStr);
	}

	bool res = GetInputFromUser_Int(&iInput,strPromt);
	
	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}
	
	status = m_pWifiTestMgr->SetPsMode(iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting power save mode.",(int)status);

		return;

	}

	sprintf(tmpStr, "Power save mode successfully set to: %d", iInput);
	DisplaySuccessMsg( tmpStr );
}
#ifndef ANDROID
void CCmdPowerSaveTriggerFrameStart::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter trigger frame sending freq in ms [1 - 1000]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 1 || iInput > 1000)
	{

		DisplayErrorMsg("Wrong Input! Input should be [1 - 1000].");

		return;

	}


	status = m_pWifiTestMgr->StartTriggerMech((uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting trigger frame mechanism.",(int)status);

		return;
	}

	char tmpStr[100];

	sprintf(tmpStr,"Trigger frame mechanism has been started. Trigger packet will be sent each :\t%d ms",iInput);

	DisplaySuccessMsg(tmpStr);

}

void CCmdPowerSaveTriggerFrameStop::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->StopTriggerMech();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error stopping trigger frame mechanism.",(int)status);

		return;
	}

	DisplaySuccessMsg("Trigger frame mechanism has been stopped");

}
#endif //ANDROID

/*void CCmdPowerSaveSetAlwaysOn::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdPowerSaveSetDoze::WifiAction()
{
	DisplayMsg("not implemented");
}

void CCmdPowerSaveSetDeepSleep::WifiAction()
{
	DisplayMsg("not implemented");
}
*/
void CCmdNuisanceGet::WifiAction()
{
	EWifiTestStatus status;

	bool filterSettings = false;

	status = m_pWifiTestMgr->GetNuisancePacketFilter(&filterSettings);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting packet filtering settings.",(int)status);

		return;

	}

	if(filterSettings == true) {

		DisplaySuccessMsg("Address filtering is enabled");

	}
	else {

		DisplaySuccessMsg("Address filtering is disabled");

	}

}

void CCmdNuisanceEnable::WifiAction()
{
	EWifiTestStatus status;

    status = m_pWifiTestMgr->SetNuisancePacketFilter(true);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error enabling packet filtering.",(int)status);

		return;

	}

	DisplaySuccessMsg("Address filtering is enabled");

}

void CCmdNuisanceDisable::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->SetNuisancePacketFilter(false);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error disabling packet filtering.",(int)status);

		return;

	}

	DisplaySuccessMsg("Address filtering is disabled");

}

void CCmdDiversityGet::WifiAction()
{

	EWifiTestStatus status;

	uint32 antennaDiversityMode = 0;

	status = m_pWifiTestMgr->GetAntennaDiversityMode(&antennaDiversityMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting diversity mode.",(int)status);

		return;

	}

	switch(antennaDiversityMode) {
		case 0:
			DisplaySuccessMsg("Antenna Div. Mode:\tMRC");
            break;
		case 1:
			DisplaySuccessMsg("Antenna Div. Mode:\tANTENNA A");
			break;
		case 2:
			DisplaySuccessMsg("Antenna Div. Mode:\tANTENNA B");
			break;
		default:
			DisplayErrorMsg("Antenna Div. Mode:\tillegal");
			break;

	}


}

void CCmdDiversityMRC::WifiAction()
{

	EWifiTestStatus status;

	status = m_pWifiTestMgr->SetAntennaDiversityMode(0);

	if(status != WIFITEST_OK) {

//		DisplayErrorMsg("Error setting diversity mode.",(int)status);
		DisplaySuccessMsg("PROBLEM !!! Antenna Diversity Mode was set to\tMRC");

		return;

	}

	DisplaySuccessMsg("Antenna Diversity Mode was set to\tMRC");

}

void CCmdDiversityAntennaA::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->SetAntennaDiversityMode(1);

	if(status != WIFITEST_OK) {

//		DisplayErrorMsg("Error setting diversity mode.",(int)status);
		DisplaySuccessMsg("PROBLEM !!! Antenna Diversity Mode was set to\tAntenna A");

		return;

	}

	DisplaySuccessMsg("Antenna Diversity Mode was set to\tAntenna A");

}

void CCmdDiversityAntennaB::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->SetAntennaDiversityMode(2);

	if(status != WIFITEST_OK) {

//		DisplayErrorMsg("Error setting diversity mode.",(int)status);
		DisplaySuccessMsg("PROBLEM !!! Antenna Diversity Mode was set to\tAntenna B");

		return;

	}

	DisplaySuccessMsg("Antenna Diversity Mode was set to\tAntenna B");

}

void CCmdMib32Get::WifiAction()
{

	EWifiTestStatus status;

	//	int iInput;
	uint32 hInput;


	std::string strPromt = "Please enter mib code (hex)>";

	bool res = GetInputFromUser_HexInt(&hInput,strPromt); 

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	uint32 iMibcode = (uint32)hInput;

	uint32 iMibValue = 0;

	status = m_pWifiTestMgr->GetMib(iMibcode,&iMibValue);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting mib value.",(int)status);

        return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Mib:\t0x%lx value is:\t%lu",iMibcode,iMibValue);

	DisplaySuccessMsg(tmpStr);

}

void CCmdMib32Set::WifiAction()
{
	EWifiTestStatus status;

	int iInput; 
	uint32 hInput;


	std::string strPromt = "Please enter mib code (hex)>";

	bool res = GetInputFromUser_HexInt(&hInput,strPromt); 

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	uint32 iMibcode = hInput; 

	strPromt = "Please enter mib value (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	status = m_pWifiTestMgr->SetMib(iMibcode,(uint32)iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting mib value.",(int)status);

		return;

	}

	DisplaySuccessMsg("Mib value was set successfully");

}

void CCmdMibMacAddrGet::WifiAction()
{

	EWifiTestStatus status;

	//int iInput;
	uint32 hInput;
	

	std::string strPromt = "Please enter mib code (hex)>";

	bool res = GetInputFromUser_HexInt(&hInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	uint32 iMibcode = (uint32)hInput;

	WmMacAddress macAddr;

	status = m_pWifiTestMgr->GetMib(iMibcode,&macAddr);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting macaddress mib value.",(int)status);

        return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Mib:\t0x%lx macadress value is:\t%2x:%2x:%2x:%2x:%2x:%2x",iMibcode,
																			macAddr.Address[0],
																			macAddr.Address[1],
																			macAddr.Address[2],
																			macAddr.Address[3],
																			macAddr.Address[4],
																			macAddr.Address[5]);

	DisplaySuccessMsg(tmpStr);

}

void CCmdMibMacAddrSet::WifiAction()
{
	EWifiTestStatus status;

	//int iInput;
	uint32 hInput;


	std::string strPromt = "Please enter mib code (hex)>";

	bool res = GetInputFromUser_HexInt(&hInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	uint32 iMibcode = hInput;

	strPromt = "Please enter Mac Address";

	uint8 pMacAddr[6];

	res = GetInputFromUser_MacAddr(pMacAddr,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	WmMacAddress wmMacAddr;

	for(int addrByte = 0;addrByte < 6;addrByte++) {

		(wmMacAddr.Address)[addrByte] = pMacAddr[addrByte];

	}

	status = m_pWifiTestMgr->SetMib(iMibcode,&wmMacAddr);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting macaddress mib value.",(int)status);

		return;

	}

	DisplaySuccessMsg("Macaddress Mib value was set successfully");

}

void CCmdMibSsidGet::WifiAction()
{

	EWifiTestStatus status;

	//int iInput;
	uint32 hInput;

	std::string strPromt = "Please enter mib code (hex)>";

	bool res = GetInputFromUser_HexInt(&hInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	uint32 iMibcode = hInput;

	WmSsid wmSsid;

	status = m_pWifiTestMgr->GetMib(iMibcode,&wmSsid);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting ssid mib value.",(int)status);

        return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Mib:\t0x%lx ssid value is:\t%s",iMibcode,wmSsid.Ssid);

	DisplaySuccessMsg(tmpStr);

}

void CCmdMibSsidSet::WifiAction()
{
	EWifiTestStatus status;

	//int iInput;
	uint32 hInput;
	

	std::string strPromt = "Please enter mib code (hex)>";

	bool res = GetInputFromUser_HexInt(&hInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	uint32 iMibcode = hInput;

	strPromt = "Please enter Ssid String";

	std::string inpStr;

	res = GetInputFromUser_String(inpStr,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(inpStr.length() > 32)
	{
		DisplayErrorMsg("Max Ssid length is 32");

		return;

	}

	WmSsid wmSsid(inpStr.c_str());

	status = m_pWifiTestMgr->SetMib(iMibcode,&wmSsid);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting ssid mib value.",(int)status);

		return;

	}

	DisplaySuccessMsg("Ssid Mib value was set successfully");

}



void CCmdDriverModeGet::WifiAction()
{

	EWifiTestStatus status;

	uint32 iMode = 0;

	status = m_pWifiTestMgr->GetDriverMode(&iMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting driver mode.",(int)status);

		return;

	}

	char tmpStr[100];

	sprintf(tmpStr,"Driver mode is:\t%lu",iMode);

	DisplaySuccessMsg(tmpStr);

}

void CCmdDriverModeSet::WifiAction()
{
	EWifiTestStatus status;

	int iInput;

	std::string strPromt = "Please enter new driver mode [0..5]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

    if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > RTL80211_DEBUG_MODE) {

		DisplayErrorMsg("Wrong Input! Input should be [0..5]");

		return;
	}

	status = m_pWifiTestMgr->SetDriverMode(iInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting driver mode.",(int)status);

		return;

	}

    DisplaySuccessMsg("Driver mode was set successfully");

}

void CCmdDriverStart::WifiAction()
{

	EWifiTestStatus status;

    status = m_pWifiTestMgr->StartDriver();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting driver.",(int)status);

		return;

	}

	DisplaySuccessMsg("Driver has started successfully");

}

void CCmdDriverStop::WifiAction()
{
	EWifiTestStatus status;

	status = m_pWifiTestMgr->StopDriver();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error stopping driver.",(int)status);

		return;

	}

	DisplaySuccessMsg("Driver has stopped successfully");


}

void CCmdRFTestTransmit::WifiAction()
{
	EWifiTestStatus status;

	CTxPacketSettings txPacketSettings;

	CWifiTestMgrPacket txPacketDefs;

	int iInput;

	bool bToStopDriver = false;

	uint32 curFreq = 0;

	status = m_pWifiTestMgr->GetTxFreq(&curFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting current frequency.",(int)status);

		return;

	}

	uint32 curMode = 0;

	status = m_pWifiTestMgr->GetDriverMode(&curMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting driver mode.",(int)status);

		return;

	}

	uint32 reqFreq = 0;

	std::string strPromt = "Please enter Tx Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	reqFreq = iInput;

	// The packet generator constantly adds 44 bytes (24 for MAC header + 20 constant bytes in the body)
	// to value it is instructed as "packetSize". The driver adds FCS of 4 bytes, so in total the MPDU
	// transmitted will vary b/w 48 and 1536

	std::stringstream ss;

	ss << "Please enter new packet size [" << MIN_PACKET_SIZE << ".." << MAX_PACKET_SIZE << "]>";

	res = GetInputFromUser_Int(&iInput,ss.str());

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < MIN_PACKET_SIZE || iInput > MAX_PACKET_SIZE)
	 {
		ss.clear();

		ss << "Wrong Input! Packet size should be [" << MIN_PACKET_SIZE << ".." << MAX_PACKET_SIZE << "]";

		DisplayErrorMsg(ss.str());

		return;
	}


	txPacketSettings.packetSize = txPacketDefs.packetSize = iInput - MIN_PACKET_SIZE;


	strPromt = "Please enter tx data rate (for long preamble (rates 2,5,11) add 128 to the rate value,for 11n use 64 + MCS Index[0-7])>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput == 1 || iInput == 2  || iInput == (0x80 + 2)  ||
	   iInput == 55 || iInput == (0x80 + 55) || iInput == 6 ||
	   iInput == 9 || iInput == 11 || iInput == (0x80 + 11) ||
	   iInput == 12 || iInput == 18 || iInput == 24  || iInput == 36 ||
	   iInput == 48 || iInput == 54 || ((iInput >= 0x40) && (iInput <= 0x47)) )
	{
		txPacketSettings.phyRate = iInput;
	}

	else
	{

		DisplayErrorMsg("Wrong Input! Invalid data rate. Valid Input is [1,2,(130),55 (183),6,9,11 (139),12,18,24,36,48,54,(64-71)].");

		return;

	}

	strPromt = "Please enter tx power level [1..63] or 0 / 64 for driver default>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 64)
	 {

		DisplayErrorMsg("Wrong Input! Input should be [0..64].");

		return;

	}



	if((iInput == 64)|| (iInput == 0))
	{
#if 0
		uint8 defTxPower;

		status = m_pWifiTestMgr->GetTxPowerLevel(reqFreq,
											     txPacketSettings.phyRate,
											     &defTxPower);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error getting default power level.",(int)status);

			return;

		}
#endif
		txPacketSettings.txPowerLevel = 0; // was defTxPower;

	}
	else
	{
		txPacketSettings.txPowerLevel = iInput;
	}


	strPromt = "Please enter packet payload type [0 - Fixed,1 - Random, 2 - Incremental]>";

	GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 2)
	 {

		DisplayErrorMsg("Wrong Input! Input should be [0 - Fixed,1 - Random, 2 - Incremental].");

		return;

	}

	txPacketSettings.payloadType = iInput;

	txPacketDefs.payloadType = iInput;

	strPromt = "Please enter number of bursts>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0..2^32]");

		return;
	}

	txPacketSettings.numTransmits = (uint32)iInput;

//Here we start configuring driver for new packet generation



	if( (reqFreq != curFreq) || (curMode != RTL80211_DEBUG_MODE) )
	{
		bToStopDriver = true;

		status = m_pWifiTestMgr->StopDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error stopping driver.",(int)status);

			return;

		}
	}

	txPacketDefs.sourceAddress[0]  = 0x66;
	txPacketDefs.sourceAddress[1]  = 0x66;
	txPacketDefs.sourceAddress[2]  = 0x66;
	txPacketDefs.sourceAddress[3]  = 0x77;
	txPacketDefs.sourceAddress[4]  = 0x77;
	txPacketDefs.sourceAddress[5]  = 0x77;

	txPacketDefs.destAddress[0] = 0x88;
	txPacketDefs.destAddress[1] = 0x88;
	txPacketDefs.destAddress[2] = 0x88;
	txPacketDefs.destAddress[3] = 0x99;
	txPacketDefs.destAddress[4] = 0x99;
	txPacketDefs.destAddress[5] = 0x99;

	txPacketDefs.bssId[0] = 0x44;
	txPacketDefs.bssId[1] = 0x44;
	txPacketDefs.bssId[2] = 0x44;
	txPacketDefs.bssId[3] = 0x55;
	txPacketDefs.bssId[4] = 0x55;
	txPacketDefs.bssId[5] = 0x55;

	txPacketDefs.broadcast=true;
	txPacketDefs.txRetryCount = 15;
	txPacketDefs.tid = 0;
	txPacketDefs.aifs=1;
	txPacketDefs.eCWmin=0;
	txPacketDefs.eCWmax=0;

	if(curMode != RTL80211_DEBUG_MODE)
	{
		status = m_pWifiTestMgr->SetDriverMode(RTL80211_DEBUG_MODE);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting driver mode.",(int)status);

			return;

		}
	}

	if(curFreq != reqFreq)
	{
		status = m_pWifiTestMgr->SetTxFreq((uint32)reqFreq);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting frequency.",(int)status);

			return;
		}
	}

	EWmConnectionState ConnectionState;

	status = m_pWifiTestMgr->GetConnectionState(&ConnectionState);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting connection state.",(int)status);

		return;

	}

	if(bToStopDriver || (ConnectionState == WM_STATE_IDLE) )
	{
		status = m_pWifiTestMgr->StartDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error starting driver.",(int)status);

			return;

		}
	}



    status = m_pWifiTestMgr->SetTxPacketParams(txPacketSettings.numTransmits,
										       txPacketSettings.phyRate,
											   txPacketSettings.txPowerLevel	,
											   &txPacketDefs);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting packet prototype.",(int)status);

		return;
	}

	status = m_pWifiTestMgr->GeneratePackets(0);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error stopping packet generation.",(int)status);

		return;
	}

	int numAttempts =0;
	//Solve problem with to continious transmits
	while(numAttempts < 3) {

		status = m_pWifiTestMgr->GeneratePackets(txPacketSettings.numTransmits);

		if(status == WIFITEST_OK) {
			break;
		}

#ifndef ANDROID
		sleep(1);
#else		
		usleep(1000);
#endif //ANDROID

		numAttempts++;

	}

	if(numAttempts == 3) {

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error generating packets.",(int)status);

			return;
		}

	}

	DisplaySuccessMsg("Starting transmit with following settings:");

	char tmpStr[100];

	sprintf(tmpStr,"NumOfTrasmits\t%lu",(uint32)iInput);


	DisplayMsg(tmpStr);

	txPacketSettings.DisplayCurrentPacketSettings();

}

void CCmdRFTestTransmitEnhanced::WifiAction()
{
	EWifiTestStatus status;

	CTxPacketSettings txPacketSettings;

	CWifiTestMgrPacket txPacketDefs;

	int iInput;
	uint8 DacOffset;

	bool bToStopDriver = false;

	uint32 curFreq = 0;

	status = m_pWifiTestMgr->GetTxFreq(&curFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting current frequency.",(int)status);

		return;

	}

	uint32 curMode = 0;

	status = m_pWifiTestMgr->GetDriverMode(&curMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting driver mode.",(int)status);

		return;

	}

	uint32 reqFreq = 0;

	std::string strPromt = "Please enter Tx Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);
	if(res == false) {

		DisplayErrorMsg("Wrong Input.");
		return;
	}


	reqFreq = iInput;

	strPromt = "Please enter new packet size [0..1536]>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 1536)
	 {

		DisplayErrorMsg("Wrong Input! Packet size should be [0..1536].");

		return;
	}

	txPacketSettings.packetSize = iInput;

	txPacketDefs.packetSize = iInput;


	strPromt = "Please enter tx data rate (for long preamble (rates 2,5,11) add 128 to the rate value,for 11n use 64 + MCS Index[0-7])>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput == 1 || iInput == 2  || iInput == (0x80 + 2)  ||
	   iInput == 55 || iInput == (0x80 + 55) || iInput == 6 ||
	   iInput == 9 || iInput == 11 || iInput == (0x80 + 11) ||
	   iInput == 12 || iInput == 18 || iInput == 24  || iInput == 36 ||
	   iInput == 48 || iInput == 54 || ((iInput >= 0x40) && (iInput <= 0x47)) )
	{
		txPacketSettings.phyRate = iInput;
	}

	else
	{

		DisplayErrorMsg("Wrong Input! Invalid data rate. Valid Input is [1,2,(130),55 (183),6,9,11 (139),12,18,24,36,48,54,(64-71)].");

		return;

	}

	strPromt = "Please enter tx power level [0..63] or 64 for driver default>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 64)
	 {

		DisplayErrorMsg("Wrong Input! Input should be [0..64].");

		return;

	}



	if((iInput == 64)|| (iInput == 0))
	{
#if 0
		uint8 defTxPower;

		status = m_pWifiTestMgr->GetTxPowerLevel(reqFreq,
											     txPacketSettings.phyRate,
											     &defTxPower);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error getting default power level.",(int)status);

			return;

		}
#endif
		txPacketSettings.txPowerLevel = 0; // was defTxPower;


	}
	else
	{
		txPacketSettings.txPowerLevel = iInput;
	}


	strPromt = "Please enter packet payload type [0 - Fixed,1 - Random, 2 - Incremental]>";

	GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 2)
	 {

		DisplayErrorMsg("Wrong Input! Input should be [0 - Fixed,1 - Random, 2 - Incremental].");

		return;

	}

	txPacketSettings.payloadType = iInput;

	txPacketDefs.payloadType = iInput;

	strPromt = "Please enter number of bursts>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0)
	{

		DisplayErrorMsg("Wrong Input! Input should be [0..2^32]");

		return;
	}

	txPacketSettings.numTransmits = (uint32)iInput;

/*
To write to the DACOFFSET in TX mode -
xpndr-console --s --p i a 1 84 48
From the example - The value for the Dacoffset is 48.
*/


	strPromt = "Please enter DACOFFSET value (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

//Note: The Write register procedure is only for: iRegType = 1; iRegAddr = 84; for other values please review.


	if (iInput > 256) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

		return;
	}

	 DacOffset=iInput;
	// The actual register updates is done later after stopping the driver and setting its mode to RTL80211_DEBUG_MODE

	// status = m_pWifiTestMgr->WriteBBRegister(regType,iRegAddr,(uint8)DacOffset);

	// Here we start configuring driver for new packet generation



	if( (reqFreq != curFreq) || (curMode != RTL80211_DEBUG_MODE) )
	{
		bToStopDriver = true;

		status = m_pWifiTestMgr->StopDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error stopping driver.",(int)status);

			return;

		}
	}

	txPacketDefs.sourceAddress[0]  = 0x66;
	txPacketDefs.sourceAddress[1]  = 0x66;
	txPacketDefs.sourceAddress[2]  = 0x66;
	txPacketDefs.sourceAddress[3]  = 0x77;
	txPacketDefs.sourceAddress[4]  = 0x77;
	txPacketDefs.sourceAddress[5]  = 0x77;

	txPacketDefs.destAddress[0] = 0x88;
	txPacketDefs.destAddress[1] = 0x88;
	txPacketDefs.destAddress[2] = 0x88;
	txPacketDefs.destAddress[3] = 0x99;
	txPacketDefs.destAddress[4] = 0x99;
	txPacketDefs.destAddress[5] = 0x99;

	txPacketDefs.bssId[0] = 0x44;
	txPacketDefs.bssId[1] = 0x44;
	txPacketDefs.bssId[2] = 0x44;
	txPacketDefs.bssId[3] = 0x55;
	txPacketDefs.bssId[4] = 0x55;
	txPacketDefs.bssId[5] = 0x55;

	txPacketDefs.broadcast=true;
	txPacketDefs.txRetryCount = 15;
	txPacketDefs.tid = 0;
	txPacketDefs.aifs=1;
	txPacketDefs.eCWmin=0;
	txPacketDefs.eCWmax=0;

	if(curMode != RTL80211_DEBUG_MODE)
	{
		status = m_pWifiTestMgr->SetDriverMode(RTL80211_DEBUG_MODE);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting driver mode.",(int)status);

			return;

		}
	}

	if(curFreq != reqFreq)
	{
		status = m_pWifiTestMgr->SetTxFreq((uint32)reqFreq);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting frequency.",(int)status);

			return;
		}
	}

//Update DACOFFSET here
	status = m_pWifiTestMgr->WriteBBRegister((ERegisterType)1,(uint32)84,(uint8)DacOffset); //Write DACOFFSET to RF register


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return;

	}




	EWmConnectionState ConnectionState;

	status = m_pWifiTestMgr->GetConnectionState(&ConnectionState);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting connection state.",(int)status);

		return;

	}

	if(bToStopDriver || (ConnectionState == WM_STATE_IDLE) )
	{
		status = m_pWifiTestMgr->StartDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error starting driver.",(int)status);

			return;

		}
	}



    status = m_pWifiTestMgr->SetTxPacketParams(txPacketSettings.numTransmits,
										       txPacketSettings.phyRate,
											   txPacketSettings.txPowerLevel	,
											   &txPacketDefs);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting packet prototype.",(int)status);

		return;
	}

	status = m_pWifiTestMgr->GeneratePackets(0);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error stopping packet generation.",(int)status);

		return;
	}

	int numAttempts =0;
	//Solve problem with to continious transmits
	while(numAttempts < 3) {

		status = m_pWifiTestMgr->GeneratePackets(txPacketSettings.numTransmits);

		if(status == WIFITEST_OK) {
			break;
		}

#ifndef ANDROID
		sleep(1);
#else		
		usleep(1000);
#endif //ANDROID

		numAttempts++;

	}

	if(numAttempts == 3) {

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error generating packets.",(int)status);

			return;
		}

	}

	DisplaySuccessMsg("Starting transmit with following settings:");

	char tmpStr[100];

	sprintf(tmpStr,"NumOfTrasmits\t%lu",(uint32)txPacketSettings.numTransmits);


	DisplayMsg(tmpStr);

	txPacketSettings.DisplayCurrentPacketSettings();

	sprintf(tmpStr,"DACOFFSET:\t%lu",(uint32)DacOffset);
	DisplayMsg(tmpStr);


}

void CCmdRFTestTransmitStop::WifiAction()
{

	EWifiTestStatus status;

	EWmConnectionState ConnectionState;

	status = m_pWifiTestMgr->GetConnectionState(&ConnectionState);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting connection state.",(int)status);

		return;

	}

	if(ConnectionState == WM_STATE_IDLE)
	{
		DisplaySuccessMsg("Stopped packet generation");

		return;

	}



	CTxPacketSettings txPacketSettings;

	CWifiTestMgrPacket txPacketDefs;

	txPacketSettings.packetSize = 100;

	txPacketDefs.packetSize = 100;

	txPacketSettings.phyRate = 36;

	txPacketSettings.txPowerLevel = 15;

	txPacketSettings.payloadType = 0;

	txPacketDefs.payloadType = 0;

	txPacketSettings.numTransmits = 0;

	txPacketDefs.sourceAddress[0]  = 0x66;
	txPacketDefs.sourceAddress[1]  = 0x66;
	txPacketDefs.sourceAddress[2]  = 0x66;
	txPacketDefs.sourceAddress[3]  = 0x77;
	txPacketDefs.sourceAddress[4]  = 0x77;
	txPacketDefs.sourceAddress[5]  = 0x77;

	txPacketDefs.destAddress[0] = 0x88;
	txPacketDefs.destAddress[1] = 0x88;
	txPacketDefs.destAddress[2] = 0x88;
	txPacketDefs.destAddress[3] = 0x99;
	txPacketDefs.destAddress[4] = 0x99;
	txPacketDefs.destAddress[5] = 0x99;

	txPacketDefs.bssId[0] = 0x44;
	txPacketDefs.bssId[1] = 0x44;
	txPacketDefs.bssId[2] = 0x44;
	txPacketDefs.bssId[3] = 0x55;
	txPacketDefs.bssId[4] = 0x55;
	txPacketDefs.bssId[5] = 0x55;

	txPacketDefs.broadcast=true;
	txPacketDefs.txRetryCount = 15;
	txPacketDefs.tid = 0;
	txPacketDefs.aifs=1;
	txPacketDefs.eCWmin=0;
	txPacketDefs.eCWmax=0;

	status = m_pWifiTestMgr->SetTxPacketParams(txPacketSettings.numTransmits,
											   txPacketSettings.phyRate,
											   txPacketSettings.txPowerLevel	,
											   &txPacketDefs);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting packet prototype.",(int)status);

		return;
	}

	status = m_pWifiTestMgr->GeneratePackets(0);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error stopping packet generation.",(int)status);

		return;
	}

	DisplaySuccessMsg("Stopped packet generation");
}


#define RATE_MASK_11A 0x7f80
#define RATE_MASK_11B 0x7f
#define RATE_MASK_11G 0x7f80

void CCmdRFTestReceive::WifiAction()
{
	EWifiTestStatus status;

	CRxSettings rxSettings;

	int iInput;

	bool bToStopDriver = false;

	uint32 curFreq = 0;

	int iMode80211 = -1;

	status = m_pWifiTestMgr->GetTxFreq(&curFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting current frequency.",(int)status);

		return;

	}

	uint32 curMode = 0;

	status = m_pWifiTestMgr->GetDriverMode(&curMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting driver mode.",(int)status);

		return;

	}


	uint32 curSupRates = 0;
	uint32 reqSupRates = RATE_MASK_11A;
#if 0
	status = m_pWifiTestMgr->GetMib(0x001b,&curSupRates);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting supported rates.",(int)status);

		return;

	}
#endif

	uint32 reqFreq = 0;

	std::string strPromt = "Please enter Tx Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	reqFreq = iInput;

	if(reqFreq > 2400 && reqFreq < 2500)
	{
		strPromt = "Please enter 80211 Mode [0 - 11B, 1 - 11G, 2 - 11BG,]>";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0 || iInput > 2)
		{
			DisplayErrorMsg("Wrong Input! Input should be [0,1,2]");

			return;

		}

		iMode80211 = iInput;

		if(iInput == 0)
		{
			reqSupRates = RATE_MASK_11B;
		}
		else if(iInput == 1)
		{
			reqSupRates = RATE_MASK_11G;
		}
		else
		{
			reqSupRates = RATE_MASK_11B + RATE_MASK_11G;;
		}



	}


	strPromt = "Please enter Mac Address for filtering [xx:xx:xx:xx:xx:xx] without brackets and spaces or (00:00:00:00:00:00 -  to unset filter)";

	uint8 pMacFilter[6];

	res = GetInputFromUser_MacAddr(pMacFilter,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	bool bIsMacFilterSet = false;

	for(int addrByte = 0;addrByte < 6;addrByte++) {

		if(pMacFilter[addrByte] != 0)
		{
			bIsMacFilterSet = true;
		}

		(rxSettings.MacFilter.Address)[addrByte] = pMacFilter[addrByte];

	}

	rxSettings.isMacFilterSet = bIsMacFilterSet;

//Here we start configuring driver for new packet generation

	if( (reqFreq != curFreq) ||
#if 0
		(curMode !=  RTL80211_DEBUG_MODE) ||
		(curSupRates != reqSupRates))
#else
		(curMode !=  RTL80211_DEBUG_MODE))
#endif
	{
		bToStopDriver = true;

		status = m_pWifiTestMgr->StopDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error stopping driver.",(int)status);

			return;

		}

	}


	if(curMode != RTL80211_DEBUG_MODE)
	{
		status = m_pWifiTestMgr->SetDriverMode(RTL80211_DEBUG_MODE);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting driver mode.",(int)status);

			return;

		}
	}

	if(curFreq != reqFreq)
	{
		status = m_pWifiTestMgr->SetTxFreq((uint32)reqFreq);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting frequency.",(int)status);

			return;
		}
	}
#if 0
	if(curSupRates != reqSupRates)
	{
		status = m_pWifiTestMgr->SetMib(0x001b,reqSupRates);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting supported rates.",(int)status);

			return;

		}
	}
#endif
	EWmConnectionState ConnectionState;

	status = m_pWifiTestMgr->GetConnectionState(&ConnectionState);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting connection state.",(int)status);

		return;

	}

	if(bToStopDriver || (ConnectionState == WM_STATE_IDLE))
	{
		status = m_pWifiTestMgr->StartDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error starting driver.",(int)status);

			return;

		}
	}


	if(rxSettings.isMacFilterSet)
	{
		status =  m_pWifiTestMgr->StartReceive(&(rxSettings.MacFilter),0);
	}
	else
	{
		status =  m_pWifiTestMgr->StartReceive(NULL,0);
	}


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting receive proc.",(int)status);

		return;

	}

	DisplaySuccessMsg("Receive proc has started.");
}


void CCmdRFTestReceiveEnhanced::WifiAction()
{
	EWifiTestStatus status;

	CRxSettings rxSettings;

	int iInput;

	bool bToStopDriver = false;

	uint32 curFreq = 0;

	int iMode80211 = -1;
	uint8 InitAgain =0;

	status = m_pWifiTestMgr->GetTxFreq(&curFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting current frequency.",(int)status);

		return;

	}

	uint32 curMode = 0;

	status = m_pWifiTestMgr->GetDriverMode(&curMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting driver mode.",(int)status);

		return;

	}


	uint32 curSupRates = 0;
	uint32 reqSupRates = RATE_MASK_11A;

	status = m_pWifiTestMgr->GetMib(0x001b,&curSupRates);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting supported rates.",(int)status);

		return;

	}


	uint32 reqFreq = 0;

	std::string strPromt = "Please enter Tx Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	reqFreq = iInput;

	if(reqFreq > 2400 && reqFreq < 2500)
	{
		strPromt = "Please enter 80211 Mode [0 - 11B, 1 - 11G, 2 - 11BG,]>";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return;
		}

		if(iInput < 0 || iInput > 2)
		{
			DisplayErrorMsg("Wrong Input! Input should be [0,1,2]");

			return;

		}

		iMode80211 = iInput;

		if(iInput == 0)
		{
			reqSupRates = RATE_MASK_11B;
		}
		else if(iInput == 1)
		{
			reqSupRates = RATE_MASK_11G;
		}
		else
		{
			reqSupRates = RATE_MASK_11B + RATE_MASK_11G;;
		}



	}


	strPromt = "Please enter Mac Address for filtering [xx:xx:xx:xx:xx:xx] without brackets and spaces or (00:00:00:00:00:00 -  to unset filter)";

	uint8 pMacFilter[6];

	res = GetInputFromUser_MacAddr(pMacFilter,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	bool bIsMacFilterSet = false;

	for(int addrByte = 0;addrByte < 6;addrByte++) {

		if(pMacFilter[addrByte] != 0)
		{
			bIsMacFilterSet = true;
		}

		(rxSettings.MacFilter.Address)[addrByte] = pMacFilter[addrByte];

	}

	rxSettings.isMacFilterSet = bIsMacFilterSet;

/*
To write to the InitAgain (RX Gain) in RX mode -
xpndr-console --s --p i a 0 65 36
From the example - The value for the RX InitAgain is 36.
*/


	strPromt = "Please enter InitAgain value (dec)>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}


	if(iInput < 0) {

		DisplayErrorMsg("Wrong Input! Input should be larger then zero");

		return;

	}

	if (iInput > 256) {

		DisplayErrorMsg("Wrong Input! Input should be in [0..256]");

		return;

	}

	// The actual register updates is done later after stopping the driver and setting its mode to RTL80211_DEBUG_MODE

	//	status = m_pWifiTestMgr->WriteBBRegister((ERegisterType)0,(unit32)65,(uint8)iInput); // For uint8 iRegType = 0; 	uint32 iRegAddr = 65;


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return;

	}
	InitAgain = iInput;

//Here we start configuring driver for new packet generation

	if( (reqFreq != curFreq) ||
		(curMode !=  RTL80211_DEBUG_MODE) ||
		(curSupRates != reqSupRates))
	{
		bToStopDriver = true;

		status = m_pWifiTestMgr->StopDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error stopping driver.",(int)status);

			return;

		}

	}


	if(curMode != RTL80211_DEBUG_MODE)
	{
		status = m_pWifiTestMgr->SetDriverMode(RTL80211_DEBUG_MODE);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting driver mode.",(int)status);

			return;

		}
	}

	if(curFreq != reqFreq)
	{
		status = m_pWifiTestMgr->SetTxFreq((uint32)reqFreq);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting frequency.",(int)status);

			return;
		}
	}

	if(InitAgain >0){

		status = m_pWifiTestMgr->WriteBBRegister((ERegisterType)0, 65, (uint8)InitAgain); // For uint8 iRegType = 0; 	uint32 iRegAddr = 65;

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error writing register.",(int)status);

			return;

		}
	}
	else {
			DisplayErrorMsg("Wrong InitAgain value.");

			return;

	}





	if(curSupRates != reqSupRates)
	{
		status = m_pWifiTestMgr->SetMib(0x001b,reqSupRates);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting supported rates.",(int)status);

			return;

		}
	}

	EWmConnectionState ConnectionState;

	status = m_pWifiTestMgr->GetConnectionState(&ConnectionState);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting connection state.",(int)status);

		return;

	}

	if(bToStopDriver || (ConnectionState == WM_STATE_IDLE) )
	{
		status = m_pWifiTestMgr->StartDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error starting driver.",(int)status);

			return;

		}
	}


	if(rxSettings.isMacFilterSet)
	{
		status =  m_pWifiTestMgr->StartReceive(&(rxSettings.MacFilter),0);
	}
	else
	{
		status =  m_pWifiTestMgr->StartReceive(NULL,0);
	}


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting receive proc.",(int)status);

		return;

	}

	DisplaySuccessMsg("Receive proc has started.");
}

void CCmdRFTestReceiveStop::WifiAction()
{
	EWifiTestStatus status;

	CWifiTestMgrRecvResults recvRes;

	status = m_pWifiTestMgr->StopReceive(&recvRes);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting received packed stats.",(int)status);

		return;

	}

	DisplaySuccessMsg("");

	CRxSettings::DisplayRxResults(&recvRes);

}

void CCmdRFTestRxGainCalibration::WifiAction()
{
	EWifiTestStatus status;


	uint8 pwr100dBm,initAGain,stepLNA,rssiDb;

	bool isExternalLNA;

	status = InitStage(&isExternalLNA);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	status = ReadRegisters(&pwr100dBm,&initAGain,&stepLNA,&rssiDb);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	double multFactor = 1.0;

	if(rssiDb == 0)
	{
		multFactor = (double)(4.0 / 3.0);
	}


	uint8 lnaGain = 1,lnaAtten = 0;

	if(isExternalLNA)
	{
		lnaAtten = 1;
	}

	char tmpStr[200];

	sprintf(tmpStr,"Starting Rx Gain Calibration with following settings:\n"
					"pwr100dBm is:\t %u\n"
					"rssiDb is:\t %u\n"
				    "initAGain is:\t %u\n"
				    "stepLNA is:\t %u\n"
				    ,pwr100dBm,rssiDb,initAGain,stepLNA);

	DisplayMsg(tmpStr);


	status = SetRegisters(lnaGain,lnaAtten,initAGain);

	sprintf(tmpStr,"Step 1 - Setting following registers:\n"
					"lnaGain is:\t %u\n"
					"lnaAtten is:\t %u\n"
				    "initAGain is:\t %u\n"
                    ,lnaGain,lnaAtten,initAGain);

	DisplayMsg(tmpStr);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	DisplayMsg("Starting the driver");

	status = m_pWifiTestMgr->StartDriver();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting driver.",(int)status);

		return;

	}

	double dRSSI0_1,dRSSI0_0,dRSSI1_1,dRSSI1_0;

	status = GetReceivedRSSI(&dRSSI0_1,&dRSSI1_1);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	DisplayMsg("Step 1 ... Done");

	lnaGain = 0;

	sprintf(tmpStr,"Step 2 - Setting following registers:\n"
					"lnaGain is:\t %u\n"
					"lnaAtten is:\t %u\n"
					"initAGain is:\t %u\n"
					,lnaGain,lnaAtten,initAGain);

	DisplayMsg(tmpStr);

	status = SetRegisters(lnaGain,lnaAtten,initAGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	status = GetReceivedRSSI(&dRSSI0_0,&dRSSI1_0);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	uint8 newInitAGain,newStepLNA;

	CalculateNewValues(dRSSI0_1,
					   dRSSI1_1,
					   dRSSI0_0,
					   dRSSI1_0,
					   multFactor,
					   initAGain,
					   stepLNA,
					   pwr100dBm,
                       &newInitAGain,
					   &newStepLNA);

	sprintf(tmpStr,"Step 2 ... DONE\n"
				   "initAGain is %u\n"
				   "stepLNA is %u\n"
				   "newInitAGain is %u\n"
				   "newStepLNA is %u\n"
				   ,initAGain,stepLNA,newInitAGain,newStepLNA);


	DisplayMsg(tmpStr);

	DisplayMsg("Saving InitAGain and StepLNA registers");

	status = SaveRegisters(newInitAGain,newStepLNA);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}


	lnaGain = 1;

	sprintf(tmpStr,"Step 3 - Setting following registers:\n"
					"lnaGain is:\t %u\n"
					"lnaAtten is:\t %u\n"
					"initAGain is:\t %u\n"
					,lnaGain,lnaAtten,newInitAGain);

	DisplayMsg(tmpStr);

	status = SetRegisters(lnaGain,lnaAtten,newInitAGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}


	status = GetReceivedRSSI(&dRSSI0_1,&dRSSI1_1);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	DisplayMsg("Step 3 ... Done");

	lnaGain = 0;

	sprintf(tmpStr,"Step 4 - Setting following registers:\n"
					"lnaGain is:\t %u\n"
					"lnaAtten is:\t %u\n"
					"initAGain is:\t %u\n"
					,lnaGain,lnaAtten,newInitAGain);

	DisplayMsg(tmpStr);

	status = SetRegisters(lnaGain,lnaAtten,newInitAGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	status = GetReceivedRSSI(&dRSSI0_0,&dRSSI1_0);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting RxGainCalibration.",(int)status);

		return;

	}

	uint8 checkInitAGain,checkStepLNA;

	CalculateNewValues(dRSSI0_1,
					   dRSSI1_1,
					   dRSSI0_0,
					   dRSSI1_0,
					   multFactor,
					   newInitAGain,
					   newStepLNA,
					   pwr100dBm,
					   &checkInitAGain,
					   &checkStepLNA);

	DisplaySuccessMsg("RX Gain Calibration has ended successfully");

	sprintf(tmpStr,"initAGain is %u\n"
					"stepLNA is %u\n"
				   "newInitAGain is %u\n"
				   "newStepLNA is %u\n"
				   "checkInitAGain is %u\n"
				   "checkStepLNA is %u\n",
					initAGain,stepLNA,
					newInitAGain,newStepLNA,
					checkInitAGain,checkStepLNA);


	DisplayMsg(tmpStr);

	return;


}

void CCmdRFTestRxGainCalibration::CalculateNewValues(double dRSSI0_1,
														double dRSSI1_1,
														double dRSSI0_0,
														double dRSSI1_0,
														double multFactor,
														uint8 initAGain,
														uint8 stepLNA,
														uint8 pwr100dBm,
														uint8 *pNewInitAGain,
														uint8 *pNewStepLNA)
{
	double deltaGainALNAGain1,deltaGainBLNAGain1,deltaGainLNAGain1;

	deltaGainALNAGain1 = -0.5*(multFactor*(double)dRSSI0_1 + (double)pwr100dBm - 56.0);

	deltaGainBLNAGain1 = -0.5*(multFactor*(double)dRSSI1_1 + (double)pwr100dBm - 56.0);


	if(deltaGainALNAGain1 > deltaGainBLNAGain1)
	{
		deltaGainLNAGain1 = deltaGainALNAGain1;
	}
	else
	{
		deltaGainLNAGain1 = deltaGainBLNAGain1;
	}

	*pNewInitAGain = initAGain + (uint32)deltaGainLNAGain1;

	*pNewStepLNA = stepLNA + (uint32)(0.5*multFactor*(dRSSI0_1 + dRSSI1_1 - dRSSI0_0 - dRSSI1_0));

}


EWifiTestStatus CCmdRFTestRxGainCalibration::InitStage(bool *pIsExternalLNA)
{
	EWifiTestStatus status;

	int iInput;

	bool bToStopDriver = false;

	uint32 curFreq = 0;

	int iMode80211 = -1;

	*pIsExternalLNA = false;

	status = m_pWifiTestMgr->GetTxFreq(&curFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting current frequency.",(int)status);

		return status;

	}

	uint32 curMode = 0;

	status = m_pWifiTestMgr->GetDriverMode(&curMode);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting driver mode.",(int)status);

		return  status;

	}


	uint32 curSupRates = 0;
	uint32 reqSupRates = RATE_MASK_11A;

	status = m_pWifiTestMgr->GetMib(0x001b,&curSupRates);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting supported rates.",(int)status);

		return status;

	}


	uint32 reqFreq = 0;

	std::string strPromt = "Please enter Freq>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return WIFITEST_ERROR;
	}

	reqFreq = iInput;

	if(reqFreq > 2400 && reqFreq < 2500)
	{
		strPromt = "Please enter 80211 Mode [0 - 11B, 1 - 11G, 2 - 11BG,]>";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {

			DisplayErrorMsg("Wrong Input.");

			return WIFITEST_ERROR;
		}

		if(iInput < 0 || iInput > 2)
		{
			DisplayErrorMsg("Wrong Input! Input should be [0,1,2]");

			return WIFITEST_ERROR;

		}

		iMode80211 = iInput;

		if(iInput == 0)
		{
			reqSupRates = RATE_MASK_11B;
		}
		else if(iInput == 1)
		{
			reqSupRates = RATE_MASK_11G;
		}
		else
		{
			reqSupRates = RATE_MASK_11B + RATE_MASK_11G;;
		}



	}


	strPromt = "Please enter Mac Address for filtering [xx:xx:xx:xx:xx:xx] without brackets and spaces or (00:00:00:00:00:00 -  to unset filter)";

	uint8 pMacFilter[6];

	res = GetInputFromUser_MacAddr(pMacFilter,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return WIFITEST_ERROR;
	}

	bool bIsMacFilterSet = false;

	for(int addrByte = 0;addrByte < 6;addrByte++) {

		if(pMacFilter[addrByte] != 0)
		{
			bIsMacFilterSet = true;
		}

		(m_RxSettings.MacFilter.Address)[addrByte] = pMacFilter[addrByte];

	}

	m_RxSettings.isMacFilterSet = bIsMacFilterSet;

	strPromt = "Please enter Lna Type [0 - external, 1 - internal]>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return WIFITEST_ERROR;
	}

	if(iInput < 0 || iInput > 1)
	{
		DisplayErrorMsg("Wrong Input! Input should be [0,1]");

		return WIFITEST_ERROR;

	}

	if(iInput == 0){

		*pIsExternalLNA = true;
	}


	strPromt = "Please enter recv proc running time [sec]>";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return WIFITEST_ERROR;
	}

	if(iInput < 1)
	{
		DisplayErrorMsg("Wrong Input! Input should be larger then 1");

		return WIFITEST_ERROR;

	}

	m_RxSettings.iTimeout = iInput;


//Here we start configuring driver for new packet generation

	if( (reqFreq != curFreq) || (curMode != RTL80211_DEBUG_MODE) || (curSupRates != reqSupRates))
	{
		bToStopDriver = true;

		status = m_pWifiTestMgr->StopDriver();

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error stopping driver.",(int)status);

			return status;

		}

	}


	if(curMode != RTL80211_DEBUG_MODE)
	{
		status = m_pWifiTestMgr->SetDriverMode(RTL80211_DEBUG_MODE);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting driver mode.",(int)status);

			return status;

		}
	}

	if(curFreq != reqFreq)
	{
		status = m_pWifiTestMgr->SetTxFreq((uint32)reqFreq);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting frequency.",(int)status);

			return status;
		}
	}

	if(curSupRates != reqSupRates)
	{
		status = m_pWifiTestMgr->SetMib(0x001b,reqSupRates);

		if(status != WIFITEST_OK) {

			DisplayErrorMsg("Error setting supported rates.",(int)status);

			return status;

		}
	}

	return WIFITEST_OK;
}

EWifiTestStatus CCmdRFTestRxGainCalibration::ReadRegisters(uint8 *pPwr100dBm,
															uint8 *pInitAGain,
															uint8 *pStepLNA,
															uint8 *pRssiDb)
{

	EWifiTestStatus status;

	uint8 pwr100dBm,initAGain,stepLNA,rssiDb;

    status =  m_pWifiTestMgr->ReadBBRegister(REGTYPE_BB,77,&pwr100dBm);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error reading register.",(int)status);

		return status;

	}


	status =  m_pWifiTestMgr->ReadBBRegister(REGTYPE_BB,65,&initAGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error reading register.",(int)status);

		return status;

	}

	status =  m_pWifiTestMgr->ReadBBRegister(REGTYPE_BB,74,&stepLNA);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error reading register.",(int)status);

		return status;

	}

	status =  m_pWifiTestMgr->ReadBBRegisterBit(REGTYPE_BB,197,4,&rssiDb);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error reading register.",(int)status);

		return status;

	}

	*pPwr100dBm = pwr100dBm;
	*pInitAGain = initAGain;
	*pStepLNA = stepLNA;
	*pRssiDb = rssiDb;

	return WIFITEST_OK;

}

EWifiTestStatus CCmdRFTestRxGainCalibration::SetRegisters(uint8 lnaGain,
														   uint8 lnaAtten,
														   uint8 initAGain)
{
	EWifiTestStatus status;

	//RxPath = 3
	status =  m_pWifiTestMgr->WriteBBRegister(REGTYPE_BB,3,3);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return status;

	}

	//Fixed Gain = 1
	status =  m_pWifiTestMgr->WriteBBRegisterBit(REGTYPE_BB,80,7,1);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return status;

	}

	//LnaGain
	status =  m_pWifiTestMgr->WriteBBRegisterBit(REGTYPE_BB,80,6,lnaGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return status;

	}

	//LnaAtten
	status =  m_pWifiTestMgr->WriteBBRegisterBit(REGTYPE_RF,95,0,lnaAtten);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return status;

	}

	//AGain = initAGain
	status =  m_pWifiTestMgr->WriteBBRegister(REGTYPE_BB,65,initAGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return status;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CCmdRFTestRxGainCalibration::SaveRegisters(uint8 newInitAGain,
															uint8 newStepLNA)
{
	EWifiTestStatus status;

	//AGain = initAGain
	status =  m_pWifiTestMgr->WriteBBRegister(REGTYPE_BB,65,newInitAGain);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return status;

	}

	status =  m_pWifiTestMgr->WriteBBRegister(REGTYPE_BB,74,newStepLNA);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error writing register.",(int)status);

		return status;

	}

	return WIFITEST_OK;
}



EWifiTestStatus CCmdRFTestRxGainCalibration::GetReceivedRSSI(double *pAvgRssiA,
															  double *pAvgRssiB)
{
	EWifiTestStatus status;

	if(m_RxSettings.isMacFilterSet)
	{
		status =  m_pWifiTestMgr->StartReceive(&(m_RxSettings.MacFilter),
											   m_RxSettings.iTimeout);
	}
	else
	{
		status =  m_pWifiTestMgr->StartReceive(NULL,m_RxSettings.iTimeout);
	}


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting receive proc.",(int)status);

		return status;

	}
#ifndef ANDROID
	sleep(m_RxSettings.iTimeout);
#else		
	usleep((m_RxSettings.iTimeout)*1000);
#endif //ANDROID



	CWifiTestMgrRecvResults recvRes;

	status = m_pWifiTestMgr->StopReceive(&recvRes);


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting received packed stats.",(int)status);

		return status;

	}

	if(recvRes.filteredReceivedWithoutError == 0)
	{
		DisplayErrorMsg("No packet received.");

		return WIFITEST_ERROR;

	}

	*pAvgRssiA = recvRes.avgRSSI_A;
	*pAvgRssiB = recvRes.avgRSSI_B;

	CRxSettings::DisplayRxResults(&recvRes);


	return WIFITEST_OK;

}

void CCmdRFTestRateMask::WifiAction()
{
	EWifiTestStatus status;

	uint32 curFreq = 0;

	uint32 reqSupRates = RATE_MASK_11A;

	int iInput;

	status = m_pWifiTestMgr->GetTxFreq(&curFreq);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting current frequency.",(int)status);

		return;

	}

	if(curFreq > 2500)
	{
		DisplaySuccessMsg("Rate mask is needed only for 802.11 B/G ");

		return;

	}

	std::string strPromt = "Please enter 80211 Mode [0 - 11B, 1 - 11G, 2 - 11BG,]>";

	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	if(iInput < 0 || iInput > 2)
	{
		DisplayErrorMsg("Wrong Input! Input should be [0,1,2]");

		return;

	}

	if(iInput == 0)
	{
		reqSupRates = RATE_MASK_11B;
	}
	else if(iInput == 1)
	{
		reqSupRates = RATE_MASK_11G;
	}
	else
	{
		reqSupRates = RATE_MASK_11B + RATE_MASK_11G;;
	}



	status = m_pWifiTestMgr->StopDriver();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error stopping driver.",(int)status);

		return;

	}


	status = m_pWifiTestMgr->SetMib(0x001b,reqSupRates);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting supported rates.",(int)status);

		return;

	}

	status = m_pWifiTestMgr->StartDriver();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error starting driver.",(int)status);

		return;

	}

}

void CCmdSysSettingGetInteger::WifiAction()
{
	CCmdSysSettingGetIntegerFromFile::GetIntegerFromFile( );
}

void CCmdSysSettingSetInteger::WifiAction()
{
	CCmdSysSettingSetIntegerInFile::SetIntegerInFile( );
}

void CCmdSysSettingGetString::WifiAction()
{
	CCmdSysSettingGetStringFromFile::GetStringFromFile( );
}

void CCmdSysSettingSetString::WifiAction()
{
	CCmdSysSettingSetStringInFile::SetStringInFile( );
}

void CCmdSysSettingGetIntegerFromFile::WifiAction()
{
	std::string fileName;

	std::string strPromt = "Please enter file name (with full path)";

	bool res = GetInputFromUser_String(fileName,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for file name");

		return;
	}
	GetIntegerFromFile( fileName.c_str() );
}

void CCmdSysSettingGetIntegerFromFile::GetIntegerFromFile(const char* FileName)
{

	ISysSettings* pSettings = SYSSETTINGS_Init( FileName );

	sint32 val = 666;

	std::string strPromt, kInput, gInput;

	strPromt = "Please enter the Group ('none' if key has no group).";

	bool res = GetInputFromUser_String(gInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for Group ");

		return;
	}

	strPromt = "Please enter the key";

	res = GetInputFromUser_String(kInput,strPromt);

    if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	uint32 sys_res;

	if( gInput == "none" )
	{
		sys_res	= pSettings->SysSettingsGetIntegerValue(kInput.c_str(),
														NULL,
														val);
	}
	else
	{
		sys_res	= pSettings->SysSettingsGetIntegerValue(kInput.c_str(),
														gInput.c_str(),
														val);
	}

	if(sys_res == 0)
	{
		uint32 len = kInput.length() + gInput.length() + 50;

		char *tmpStr = new char[len];

		if(tmpStr)
		{
			sprintf(tmpStr,"value in group = %s, key %s  is :\t%d", gInput.c_str(),
																	kInput.c_str(),
																	(int)val);

			DisplaySuccessMsg(tmpStr);

			delete tmpStr;
		}
		else
		{
			DisplayErrorMsg("Cannot get integer value from DB - memory problem");
		}


	}
	else
	{
		DisplayErrorMsg("Cannot get integer value from DB");
	}

	SYSSETTINGS_Terminate();
}



void CCmdSysSettingSetIntegerInFile::WifiAction()
{

	std::string fileName;

	std::string strPromt = "Please enter file name (with full path)";

	bool res = GetInputFromUser_String(fileName,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for file name");

		return;
	}

	SetIntegerInFile( fileName.c_str() );
}

void CCmdSysSettingSetIntegerInFile::SetIntegerInFile(const char* FileName)

{

	ISysSettings* pSettings = SYSSETTINGS_Init( FileName );

	std::string strPromt, kInput, gInput;

	strPromt = "Please enter the Group ('none' if key has no group).";

	bool res = GetInputFromUser_String(gInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for Group ");

		return;
	}

	strPromt = "Please enter the key";

	res = GetInputFromUser_String(kInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	int iInput;

	strPromt = "Please enter the value";

	res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	uint32 sys_res;

	if( gInput == "none" )
	{
		sys_res	= pSettings->SysSettingsSetIntegerValue(kInput.c_str(),
														NULL,
														 iInput);
	}
	else
	{
		sys_res	= pSettings->SysSettingsSetIntegerValue(kInput.c_str(),
														gInput.c_str(),
														 iInput);
	}

	if(sys_res == 0)
	{
		uint32 len = kInput.length() + gInput.length() + 50;

		char *tmpStr = new char[len];

		if(tmpStr)
		{
			sprintf(tmpStr,"value in group = %s, key %s  was set to :\t%d", gInput.c_str(),
																			kInput.c_str(),
																			iInput);

			DisplaySuccessMsg(tmpStr);

			delete tmpStr;
		}
		else
		{
			DisplayErrorMsg("Cannot set integer value in DB - memory problem");
		}

	}
	else
	{
		DisplayErrorMsg("Cannot set integer value in DB");
	}

	SYSSETTINGS_Terminate();
}

void CCmdSysSettingGetStringFromFile::WifiAction()
{
	std::string fileName;

	std::string strPromt = "Please enter file name (with full path)";

	bool res = GetInputFromUser_String(fileName,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for file name");

		return;
	}
	GetStringFromFile( fileName.c_str() );
}



void CCmdSysSettingGetStringFromFile::GetStringFromFile(const char* FileName)

{

	ISysSettings* pSettings = SYSSETTINGS_Init( FileName );

	std::string strPromt, kInput, gInput;

	strPromt = "Please enter the Group ('none' if key has no group).";

	bool res = GetInputFromUser_String(gInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for Group ");

		return;
	}

	strPromt = "Please enter the key";

	res = GetInputFromUser_String(kInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	char *val = NULL;

	uint32 sys_res;

	if( gInput == "none" )
	{
		sys_res	= pSettings->SysSettingsGetStringValue(kInput.c_str(),
													   NULL,
													   (const char**)&val);
	}
	else
	{
		sys_res	= pSettings->SysSettingsGetStringValue(kInput.c_str(),
													   gInput.c_str(),
													   (const char**)&val);
	}

	if(sys_res == 0)
	{

		uint32 len = strlen(val) + kInput.length() + gInput.length() + 50;

		char *tmpStr = new char[len];

		if(tmpStr)
		{
			sprintf(tmpStr,"value in group = %s, key %s  is :\t%s", gInput.c_str(),
																	kInput.c_str(),
																	val);

			DisplaySuccessMsg(tmpStr);

			delete tmpStr;
		}
		else
		{
			DisplayErrorMsg("Cannot get string value from DB - memory problem");
		}
	}
	else
	{
		DisplayErrorMsg("Cannot get string value from DB");
	}

	SYSSETTINGS_Terminate();
}

void CCmdSysSettingSetStringInFile::WifiAction()
{
	std::string fileName;

	std::string strPromt = "Please enter file name (with full path)";

	bool res = GetInputFromUser_String(fileName,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for file name");

		return;
	}

	SetStringInFile(fileName.c_str());

}


void CCmdSysSettingSetStringInFile::SetStringInFile(const char* FileName)
{

	ISysSettings* pSettings = SYSSETTINGS_Init( FileName );

	std::string kInput, gInput, vInput, strPromt;

	strPromt = "Please enter the Group ('none' if key has no group).";

	bool res = GetInputFromUser_String(gInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for Group ");

		return;
	}

	strPromt = "Please enter the key";

	res = GetInputFromUser_String(kInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	strPromt = "Please enter the string value";

	res = GetInputFromUser_String(vInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input for value.");

		return;
	}

	uint32 sys_res;

	if( gInput == "none" )
	{
		sys_res	= pSettings->SysSettingsSetStringValue(kInput.c_str(),
													   NULL,
													   vInput.c_str());
	}
	else
	{
		sys_res	= pSettings->SysSettingsSetStringValue(kInput.c_str(),
													   gInput.c_str(),
													   vInput.c_str());
	}

	if(sys_res == 0)
	{

		uint32 len = vInput.length() + kInput.length() + gInput.length() + 50;

		char *tmpStr = new char[len];

		if(tmpStr)
		{
			sprintf(tmpStr,"value in group = %s, key %s was set to :\t%s", gInput.c_str(),
																		   kInput.c_str(),
																		   vInput.c_str());
			DisplaySuccessMsg(tmpStr);

			delete tmpStr;
		}
		else
		{
			DisplayErrorMsg("Cannot set string value in DB - memory problem");
		}

	}
	else
	{
		DisplayErrorMsg("Cannot set string value in DB");
	}

	SYSSETTINGS_Terminate();
}



void CCmdSysSettingGetBoardId::WifiAction()
{
    uint8 iBoardId =0;
	EWifiTestStatus status = m_pWifiTestMgr->GetBoardId(&iBoardId);

	if(status != WIFITEST_OK) {
		DisplayErrorMsg("Error Getting Board ID (%d).",(int)status);
		return;
	}

	char tmpStr[100];
	sprintf(tmpStr,"Board ID value is:\t%lu",(uint32)iBoardId);

	DisplaySuccessMsg(tmpStr);
}

void CCmdSysSettingGetBoardRfParam::WifiAction()
{
	uint8 iBoardId =0;
	EWifiTestStatus status = m_pWifiTestMgr->GetBoardId(&iBoardId);

	if(status != WIFITEST_OK) {
		DisplayErrorMsg("Error Getting Board ID (%d).",(int)status);
		return;
	}

	char strBoardId[100];
	sprintf(strBoardId,"%lu",(uint32)iBoardId);

	ISysSettings* pSettings = SYSSETTINGS_Init( "/etc/syssettings/rfboardssettings.sqlite" );

	char* antenna;

	uint32 sys_res = pSettings->SysSettingsGetStringValue("RxAntenna",
														strBoardId,
														(const char**)&antenna);

	if( sys_res !=0 )
	{
		DisplayErrorMsg("Error Getting Rx Antenna value. (Board Id = %d)",iBoardId );
		return;
	}

	sint32 externalLna = 0;
	sys_res = pSettings->SysSettingsGetIntegerValue( "ExternalLna",
													 strBoardId,
													 externalLna );

	if( sys_res !=0 )
	{
		DisplayErrorMsg("Error Getting External LNA value.(Board Id = %d)",iBoardId);
		return;
	}

	char tmpStr[100];
	sprintf(tmpStr,"Board ID:\t%lu\nRx Antenna:\t%s\nExternal LNA:\t%d",
			(uint32)iBoardId, antenna,(int)externalLna );

	DisplaySuccessMsg(tmpStr);

}

void CCmdSysSettingSyncInteger::WifiAction()
{
}

char* CCmdRfCalibration::GetValue(const char* group, const char *key)
{
	ISysSettings* pSettings = SYSSETTINGS_Init( NULL );

	char *val;

	uint32 sys_res = pSettings->SysSettingsGetStringValue(key,
														  group,
														  (const char**)&val);
	SYSSETTINGS_Terminate();

	if(sys_res == 0)
	{
		return val;
	}
	else
	{
		return NULL;
	}
}

int CCmdRfCalibration::ParseLine(vector<uint32>& valVector,
								 const std::string& field)
{
	stringstream recStream(field);

	string tmpField;

	uint32 curVal;

	int iRes;

	while(getline(recStream,tmpField,','))
	{
		iRes = ConvertField(tmpField,&curVal);

		if(iRes < 0)
		{
			return -1;
		}

		valVector.push_back(curVal);
	}

	return 0;

}

int CCmdRfCalibration::ConvertField(const std::string& field,uint32* pCurVal)
{

	stringstream ss(field);

	ss >> *pCurVal;

    if(!std::cin)
	{
	  return -1;
	}

	return 0;

}

void CCmdRfCalibration::LoadDataForDebug()
{
	/// only for debug!!!
	const char* GROUP_NAMES[100] = {"RfCalib_DefaultRegisters",
								    "RfCalib_RegistersByBand",
									"RfCalib_RegistersByChannel"};
	const char* MASTER_KEY = "RegistersList";

	ISysSettings* pSettings = SYSSETTINGS_Init( NULL );
	pSettings->SysSettingsSetStringValue(MASTER_KEY, GROUP_NAMES[0], "1,2");
	pSettings->SysSettingsSetStringValue("Reg1" , GROUP_NAMES[0], "1,0,30");
	pSettings->SysSettingsSetStringValue("Reg2" , GROUP_NAMES[0], "2,0,60");
	pSettings->SysSettingsSetStringValue(MASTER_KEY , GROUP_NAMES[1], "65,4");
	pSettings->SysSettingsSetStringValue("Reg65" , GROUP_NAMES[1], "65,0,255,11,21,31,41,51,61,71,81,91");
	pSettings->SysSettingsSetStringValue("Reg4" , GROUP_NAMES[1], "4,0,255,10,20,30,40,50,60,70,80,90");
	pSettings->SysSettingsSetStringValue(MASTER_KEY , GROUP_NAMES[2], "5,6");
	pSettings->SysSettingsSetStringValue("Reg5" , GROUP_NAMES[2], "5,0,255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7");
	pSettings->SysSettingsSetStringValue("Reg6" , GROUP_NAMES[2], "6,0,255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,1,2,3,4,5,6,7");
	SYSSETTINGS_Terminate();
	///////////////////////////
}

int CCmdRfCalibration::LoadGroup(IN const char* strGroupName,
								 IN EWmMibPhyParameterType type,
								 IN uint32 validEntryLength,
								  OUT WmMibPhyParameter &stPhyParam)
{
	vector<uint32> regAddrVector, valVector;
	vector<uint32>::const_iterator regAddrVector_it, val_it;
    char *registerVal;
	char *registersList;
	char registerIndex[10];

	stPhyParam.paramType = type;
	stPhyParam.dataSize = 0;
	stPhyParam.data = NULL;

	registersList = GetValue(strGroupName,
							 "RegistersList");

	if(registersList == NULL) {
		printf("GroupName %s - Load Group -1 \n",strGroupName);
		return 0;
	}

	regAddrVector.clear();

	int iRes = ParseLine(regAddrVector, registersList);

	if(iRes < 0)
	{
		DisplayErrorMsg("Cannot parse Registers list");

		return -1;
	}

	if(regAddrVector.size() == 0)
	{
		printf("Load Group -2 ");
		return 0;
	}

	stPhyParam.data = new uint8[regAddrVector.size() * validEntryLength ];

	uint32 vals_index = 0;

	for (regAddrVector_it = regAddrVector.begin();
		 regAddrVector_it != regAddrVector.end();
		 regAddrVector_it++)
	{

		sprintf(registerIndex,"Reg%lu",(uint32)(*regAddrVector_it));

		registerVal = GetValue(strGroupName,
							   registerIndex);

		if(registerVal == NULL)
		{
			printf("Load Group -3 ");
			continue;

		}

		valVector.clear();

		iRes = ParseLine(valVector, registerVal);

		if(iRes < 0)
		{
			DisplayErrorMsg("Cannot parse register values");

			return -1;
		}

		if(valVector.size() !=  validEntryLength) {
			printf("Load Group -4 ");
			continue;
		}

		for (val_it = valVector.begin(); val_it != valVector.end(); val_it++)
		{
			stPhyParam.data[vals_index++] = (uint8)(*val_it);
		}
	}

	stPhyParam.dataSize = vals_index;

	//tempdDebug

	printf("Parsing group results:");
	printf("data_size=%lu\n",stPhyParam.dataSize);
	printf("paramType=%lu\n",stPhyParam.paramType);

	if(stPhyParam.dataSize >  0) {

		for(uint32 j=0 ; j < stPhyParam.dataSize-1 ; j++)
		{
			printf("%u,",(uint16)(stPhyParam.data[j]));
		}

		printf("%d\n",stPhyParam.data[stPhyParam.dataSize-1]);
	}

	return 0;
}

void CCmdRfCalibration::WifiAction()
{
	const char* GROUP_NAMES[100] = {"RfCalib_DefaultRegisters",
									"RfCalib_RegistersByBand",
									"RfCalib_RegistersByChannel24",
									"RfCalib_RegistersByChannel"};

	const uint32 GROUP_ENTRY_LENGTH[4] = {3,12,17,259};

	EWifiTestStatus status;

	WmMibPhyParameter stArrWmMibPhyParameter[4];

	stArrWmMibPhyParameter[0].data = NULL;
	stArrWmMibPhyParameter[1].data = NULL;
	stArrWmMibPhyParameter[2].data = NULL;
	stArrWmMibPhyParameter[3].data = NULL;

	//LoadDataForDebug();

	int iRes = LoadGroup(GROUP_NAMES[0],
						 WM_PHYPARAM_DEFAULTREGISTERS,
						 GROUP_ENTRY_LENGTH[0],
						 stArrWmMibPhyParameter[0]);

	if(iRes != 0)
	{
		goto error_handle;
	}

	iRes = LoadGroup(GROUP_NAMES[1],
					  WM_PHYPARAM_REGISTERSBYBAND,
					  GROUP_ENTRY_LENGTH[1],
					  stArrWmMibPhyParameter[1]);

	if(iRes != 0)
	{
		goto error_handle;
	}

	iRes = LoadGroup(GROUP_NAMES[2],
						 WM_PHYPARAM_REGISTERSBYCHANL24,
						 GROUP_ENTRY_LENGTH[2],
						 stArrWmMibPhyParameter[2]);

	if(iRes != 0)
	{
		goto error_handle;
	}

	iRes = LoadGroup(GROUP_NAMES[3],
						 WM_PHYPARAM_REGISTERSBYCHANL,
						 GROUP_ENTRY_LENGTH[3],
						 stArrWmMibPhyParameter[3]);

	if(iRes != 0)
	{
		goto error_handle;
	}

	status = m_pWifiTestMgr->StopDriver();

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting Phy parameters - Error stopping driver.",(int)status);

		goto error_handle;

	}

	if(stArrWmMibPhyParameter[0].dataSize > 0)
	{
		status = m_pWifiTestMgr->SetPhyParameters(&stArrWmMibPhyParameter[0]);

		if(status != WIFITEST_OK)
		{

            DisplayErrorMsg("Error setting Phy parameters - default registers values",(int)status);

			goto error_handle;

		}

	}

	if(stArrWmMibPhyParameter[1].dataSize > 0)
	{
		status = m_pWifiTestMgr->SetPhyParameters(&stArrWmMibPhyParameter[1]);

		if(status != WIFITEST_OK)
		{

			DisplayErrorMsg("Error setting Phy parameters - default registers by band",(int)status);

			goto error_handle;

		}

	}
	if(stArrWmMibPhyParameter[2].dataSize > 0)
	{
		status = m_pWifiTestMgr->SetPhyParameters(&stArrWmMibPhyParameter[2]);

		if(status != WIFITEST_OK)
		{

			DisplayErrorMsg("Error setting Phy parameters - default registers by channel 2.4G",(int)status);

			goto error_handle;

		}

	}

	if(stArrWmMibPhyParameter[3].dataSize > 0)
	{
		status = m_pWifiTestMgr->SetPhyParameters(&stArrWmMibPhyParameter[3]);

		if(status != WIFITEST_OK)
		{

			DisplayErrorMsg("Error setting Phy parameters - default registers by channel",(int)status);

			goto error_handle;

		}

	}

	DisplaySuccessMsg("Rf Calibration Done");

error_handle:

	if(stArrWmMibPhyParameter[0].data != NULL) {
		delete stArrWmMibPhyParameter[0].data;
	}

	if(stArrWmMibPhyParameter[1].data != NULL) {
		delete stArrWmMibPhyParameter[1].data;
	}

	if(stArrWmMibPhyParameter[2].data != NULL) {
		delete stArrWmMibPhyParameter[2].data;
	}

}

void CCmdDBGMsgGetInteger::WifiAction()
{
	EWifiTestStatus status;
	uint32 iMibcode = 0x5d; // MIBOID_DBGFILTERFLAGS;
	uint32 iMibValue = 0;
	char tmpStr[100];

	status = m_pWifiTestMgr->GetMib(iMibcode,&iMibValue);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting mib value.",(int)status);

        return;

	}

	sprintf(tmpStr,"DbgMsg Filer value (hex) is:\t%lx",iMibValue);

	DisplaySuccessMsg(tmpStr);

	iMibcode = 0x5c; // MIBOID_DBGMODULEFLAGS;

	iMibValue = 0;

	status = m_pWifiTestMgr->GetMib(iMibcode,&iMibValue);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting mib value.",(int)status);

        return;

	}

	sprintf(tmpStr,"DbgMsg Module value (hex) is:\t%lx",iMibValue);

	DisplaySuccessMsg(tmpStr);


}

void CCmdDBGMsgSetInteger::WifiAction()
{
	EWifiTestStatus status;
	//int iInput;
	uint32 hInput; 

	uint32 iMibcode = 0x5d; // MIBOID_DBGFILTERFLAGS;
	char tmpStr[3000];

	sprintf(tmpStr,
		"\tDbgFilter\n"
		"\t==================\n"
		"\tDW_INFO		BIT(0)\n"
		"\tDW_WARN		BIT(1)\n"
		"\tDW_ERR		BIT(2)\n"
		"\tDW_TX		BIT(3)\n"
		"\tDW_RX		BIT(4)\n"
		"\tDW_MGMT		BIT(5)\n"
		"\tDW_QOS		BIT(6)\n"
		"\tDW_DEBUG	BIT(15)\n");
	DisplayMsg(tmpStr);


	std::string strPromt = "Please enter DbgFilter value (hex)>";

	bool res = GetInputFromUser_HexInt(&hInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	status = m_pWifiTestMgr->SetMib(iMibcode,(uint32)hInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting mib value.",(int)status);

		return;

	}

	DisplaySuccessMsg("DbgFilter value was set successfully");

	iMibcode = 0x5c; // MIBOID_DBGMODULEFLAGS;

		sprintf(tmpStr,
		"\tDbgModules\n"
		"\t========================================================\n"
		"\tDW_GEN 	\tBIT(0),	 General Utilities and other misc.\n"
		"\tDW_SME 	\tBIT(1),	 Station Managament Entity\n"
		"\tDW_OSW 	\tBIT(2),	 OS Wrapper\n"
		"\tDW_DBM 	\tBIT(3),	 Descriptor and Buffer Management\n"
		"\tDW_ENCAP \tBIT(4),	 Encapsulation\n"
		"\tDW_SEC 	\tBIT(5),	 Security\n"
		"\tDW_11H 	\tBIT(6),	 802.11h\n"
		"\tDW_RATEPWR \tBIT(7),	 Rate and Power adjustment\n"
		"\tDW_MIB 	\tBIT(8),	 Management Information Base\n"
		"\tDW_LWAM \tBIT(9),	 Local WAM\n"
		"\tDW_SIG 	\tBIT(10), Signaling\n"
		"\tDW_PSM 	\tBIT(11), Power Save Manager\n"
		"\tDW_DECAP \tBIT(12), Decapsulation\n"
		"\tDW_SCH 	\tBIT(13), Scheduler\n"
		"\tDW_RXFILTER \tBIT(14), RX Filter\n"
		"\tDW_HAL 	\tBIT(15), Hardware Abstraction Layer\n"
		"\tDW_LWE 	\tBIT(16), Linux Wireless Extenstions\n"
		"\tDW_PKTCLS \tBIT(17), Packet Classifier\n"
		"\tDW_RPA 	\tBIT(18), Rate Power Adjust\n"
		"\tDW_EQSM \tBIT(19), Event Queue and State machine\n");

		DisplayMsg(tmpStr);


	strPromt = "Please enter DbgModules value (hex)>";

	res = GetInputFromUser_HexInt(&hInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}

	status = m_pWifiTestMgr->SetMib(iMibcode,(uint32)hInput);

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting mib value.",(int)status);

		return;

	}

	DisplaySuccessMsg("DbgModules value was set successfully");


}


void CCmdRFTestMonitor::WifiAction()
{


	EWifiTestStatus status;
	uint8 TMRateLUT[] = {1,2,55,6,9,11,12,18,24,36,48,54};
	uint32 NumberOfPacketsPerTemp;
	uint32 iMibValue = 0;


	WmMibTMdata wmTMdata;
	char tmpStr[500];

	status = m_pWifiTestMgr->GetTMdata(&wmTMdata); //MIBOID_TM_MONITOR_DATA

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting TM_MONITOR_DATA mib value.",(int)status);

        return;

	}

	status = m_pWifiTestMgr->GetMib((uint32)0x0085,&iMibValue); //#define MIBOID_TM_MONITOR_CONTROL	0X0085


	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error getting TM_MONITOR_CONTROL mib value.",(int)status);

        return;
	}

	if ((iMibValue & 0x0ffffff) == 0 ) { // TM Monitor is not active
		sprintf(tmpStr,
		"\t\tMonitoring is off\n");
		DisplayMsg(tmpStr);
		return;

	}
	

/*
	DSPG_TM_DATA_INVALID, =0
	DSPG_TM_DATA_RX, =1
	DSPG_TM_DATA_TX, =2
 */

	NumberOfPacketsPerTemp =wmTMdata.NumberOfPAckets;

	if (wmTMdata.type == 1) //Receive
	{
		sprintf(tmpStr,
		"========================================================\n"
		"\t\tReceive - Statistics\n"
		"========================================================\n"
		"FCS Error Rate (ALL Packets) [percent]:\t%lu\n",wmTMdata.RxFCSErr);
		DisplayMsg(tmpStr);
		sprintf(tmpStr,
			"Mean RSSI-A:\t%d [dbm]\n"
			"Mean RSSI-B:\t%d [dbm]\n"
			,(int)(wmTMdata.RxAccRssiA/wmTMdata.NumberOfPAckets-100)
			,(int)(wmTMdata.RxAccRssiB/wmTMdata.NumberOfPAckets-100));
		DisplayMsg(tmpStr);
	}
	else if (wmTMdata.type == 2) //Transmit
	{
		sprintf(tmpStr,
		"========================================================\n"
		"\t\tTransmit - Statistics\n"
		"========================================================\n"
		"PER [percent]:\t%lu\n",100-(100*wmTMdata.TxAccOK)/wmTMdata.NumberOfPAckets);
		DisplayMsg(tmpStr);
		sprintf(tmpStr,
			"Number of Retries:\t%d \n"
			"Number of Fails:\t%d \n"
			,(int)(wmTMdata.TxNAtmp)
			,(int)(wmTMdata.TxFail));
		DisplayMsg(tmpStr);
		
	}
	else
	{
		sprintf(tmpStr,
		"\t\tNo Data\n");
		DisplayMsg(tmpStr);
		return;
	}
		
	//Draw Histogram
	uint8 Tempi, Tempj,TempStri;
	TempStri=0;
	sprintf(tmpStr, "----------- HISTOGRAM (Npackets = %lu) -----", wmTMdata.NumberOfPAckets);
	DisplayMsg(tmpStr);
	
	for (Tempi = 0; Tempi < 12 ; Tempi++)
	{
//		sprintf(tmpStr ,"[%2d]--[%5d]|",TMRateLUT[Tempi],wmTMdata.RateHistogram[Tempi]);
		sprintf(tmpStr ,"[%2d] ",TMRateLUT[Tempi]);
		for (Tempj = 0; (Tempj < (30*wmTMdata.RateHistogram[Tempi])/NumberOfPacketsPerTemp) ; Tempj++)
		{
			sprintf(tmpStr + strlen(tmpStr),"*");
		}
		if (wmTMdata.RateHistogram[Tempi] > 0) {
			sprintf(tmpStr + strlen(tmpStr),"(%d)",wmTMdata.RateHistogram[Tempi]);
		}
		
		
	DisplayMsg(tmpStr);
	}
	sprintf(tmpStr ,"-----------------------------------------------");

	DisplayMsg(tmpStr);


/*
	DW_TM_RATE_1_Mb,
	DW_TM_RATE_2_Mb,
	DW_TM_RATE_55_Mb,
	DW_TM_RATE_6_Mb,
	DW_TM_RATE_9_Mb,
	DW_TM_RATE_11_Mb,
	DW_TM_RATE_12_Mb,
	DW_TM_RATE_18_Mb,
	DW_TM_RATE_24_Mb,
	DW_TM_RATE_36_Mb,
	DW_TM_RATE_48_Mb,
	DW_TM_RATE_54_Mb,
	DW_TM_RATE_LAST_INDEX
*/

}



void CCmdRFTestMonitorControl::WifiAction()
{
	EWifiTestStatus status;
	uint32 ControlWord;
	int iInput;

	
	char tmpStr[500];
	ControlWord = 0;
/*
#define TM_MONITOR_IS_RX_SELECTED 		0x00010000
#define TM_MONITOR_IS_TX_SELECTED 		0x00020000
#define TM_MONITOR_RESET_FLAG			0x10000000
#define TM_MONITOR_MAX_PACKETS_MASK 	0x0000ffff
#define TM_MONITOR_STOP					0x00000000
*/

	std::string strPromt = "Please enter Monitoring Action: Stop - 0 / Rx - 1 / Tx - 2 ";


	bool res = GetInputFromUser_Int(&iInput,strPromt);

	if(res == false) {

		DisplayErrorMsg("Wrong Input.");

		return;
	}
	else if (iInput > 2) {
		DisplayErrorMsg("Wrong Input - Must be: 0 / 1 / 2");
		return;
	}

	ControlWord += (uint32)(iInput * 0x10000 + 0x10000000); // Set action and force reset

	if (iInput > 0) { // If action is not stop
		strPromt = "Maximum Number of Packets: >";

		res = GetInputFromUser_Int(&iInput,strPromt);

		if(res == false) {
	
			DisplayErrorMsg("Wrong Input.");

			return;
		}
		else if (iInput > 65535) {
			DisplayErrorMsg("Wrong Input - Must lower than 65535");
			return;
		}

		ControlWord += (uint32)iInput; // Set action and force reset
	}

	//	sprintf(tmpStr,"===> MIB Value: %lx\n", ControlWord);
	//	DisplayMsg(tmpStr);
	

	status = m_pWifiTestMgr->SetMib((uint32)0x85,ControlWord); //MIBOID_TM_MONITOR_CONTROL

	if(status != WIFITEST_OK) {

		DisplayErrorMsg("Error setting mib value.",(int)status);

		return;

	}

	DisplaySuccessMsg("TM Monitor value was set successfully");
}

	


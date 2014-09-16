#ifndef MENUCOMMANDS_H
#define MENUCOMMANDS_H

#include "TypeDefs.h"
#include "WifiCLICommand.h"
#include "TxPacketSettings.h"
#include "RxSettings.h"
#include  <vector>

class CCmdPhyRateAdoptGet : public CWifiCliCommand
{
public:
	CCmdPhyRateAdoptGet(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}
	virtual ~CCmdPhyRateAdoptGet() {}

	virtual void WifiAction();

};

class CCmdPhyRateAdoptEnable : public CWifiCliCommand
{
public:
	CCmdPhyRateAdoptEnable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPhyRateAdoptEnable() {}


	virtual void WifiAction();

};

class CCmdPhyRateAdoptDisable : public CWifiCliCommand
{
public:
	CCmdPhyRateAdoptDisable(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPhyRateAdoptDisable() {}

	virtual void WifiAction();

};

class CCmdMaxPhyRateGet : public CWifiCliCommand
{
public:
	CCmdMaxPhyRateGet(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMaxPhyRateGet() {}

	virtual void WifiAction();

};


class CCmdMaxPhyRateSet : public CWifiCliCommand
{
public:
	CCmdMaxPhyRateSet(CWifiTestMgr* pCWifiTestMgr,
					  const std::string & name,
					  const std::string & description,
					  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMaxPhyRateSet() {}

	virtual void WifiAction();

};


class CCmdFreqGet : public CWifiCliCommand
{
public:
	CCmdFreqGet(CWifiTestMgr* pCWifiTestMgr,
				const std::string & name,
				const std::string & description,
				uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
											  name,
											  description,
											  shortcut) {}

	virtual ~CCmdFreqGet() {}

	virtual void WifiAction();

};


class CCmdFreqSet : public CWifiCliCommand
{
public:
	CCmdFreqSet(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdFreqSet() {}

	virtual void WifiAction();

};

class CCmdFreqGetScanListChanFreq : public CWifiCliCommand
{
public:
	CCmdFreqGetScanListChanFreq(CWifiTestMgr* pCWifiTestMgr,
								const std::string & name,
								const std::string & description,
								uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
																name,
																description,
																shortcut) {}

	virtual ~CCmdFreqGetScanListChanFreq() {}

	virtual void WifiAction();

};

class CCmdFreqSetScanListChannel : public CWifiCliCommand
{
public:
	CCmdFreqSetScanListChannel(CWifiTestMgr* pCWifiTestMgr,
								const std::string & name,
								const std::string & description,
								uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
																name,
																description,
																shortcut) {}

	virtual ~CCmdFreqSetScanListChannel() {}

	virtual void WifiAction();

};

class CCmdFreqAddScanListChannel : public CWifiCliCommand
{
public:
	CCmdFreqAddScanListChannel(CWifiTestMgr* pCWifiTestMgr,
								const std::string & name,
								const std::string & description,
								uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
																name,
																description,
																shortcut) {}

	virtual ~CCmdFreqAddScanListChannel() {}

	virtual void WifiAction();

};


class CCmdPowerMaxGet : public CWifiCliCommand
{
public:
	CCmdPowerMaxGet(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerMaxGet() {}

	virtual void WifiAction();

};


class CCmdPowerMaxSet : public CWifiCliCommand
{
public:
	CCmdPowerMaxSet(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerMaxSet() {}

	virtual void WifiAction();

};


class CCmdPowerCurGet : public CWifiCliCommand
{
public:
	CCmdPowerCurGet(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerCurGet() {}

	virtual void WifiAction();

};


class CCmdPowerCurSetAll : public CWifiCliCommand
{
public:
	CCmdPowerCurSetAll(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerCurSetAll() {}

	virtual void WifiAction();

};


class CCmdPowerCurSet : public CWifiCliCommand
{
public:
	CCmdPowerCurSet(CWifiTestMgr* pCWifiTestMgr,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerCurSet() {}

	virtual void WifiAction();

};


class CCmdPowerCurLoadTable : public CWifiCliCommand
{
public:
	CCmdPowerCurLoadTable(CWifiTestMgr* pCWifiTestMgr,
							const std::string & name,
							const std::string & description,
							uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
														  name,
														  description,
														  shortcut) {}

	virtual ~CCmdPowerCurLoadTable() {}

	virtual void WifiAction();

private:

	int ConvertField(const std::string& field,uint32* pCurVal);

	int ParseLine(vector<uint32>& valVector,
				  const std::string& field);

	int UpdatePowerLevelTable(vector<uint32>& valVector,
		 					  WmPowerLevelTable* pPowerLevelTable);

	int ParsePowerTableFile(ifstream& fs,
						    WmPowerLevelTable* pPowerLevelTable,
							uint32* pErrLineNum);

};

class CCmdPacketGenGet : public CWifiCliCommand
{
public:
	CCmdPacketGenGet(CWifiTestMgr* pCWifiTestMgr,
					 CTxPacketSettings* pTxPacketSettings,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pTxPacketSettings(pTxPacketSettings) {}

	virtual ~CCmdPacketGenGet() {}

	virtual void WifiAction();

private:

	CTxPacketSettings* m_pTxPacketSettings;

};



class CCmdPacketGenSetPacketSize : public CWifiCliCommand
{
public:
	CCmdPacketGenSetPacketSize(CWifiTestMgr* pCWifiTestMgr,
					 CTxPacketSettings* pTxPacketSettings,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pTxPacketSettings(pTxPacketSettings) {}

	virtual ~CCmdPacketGenSetPacketSize() {}

	virtual void WifiAction();

private:

	CTxPacketSettings* m_pTxPacketSettings;

};


class CCmdPacketGenSetPacketRate : public CWifiCliCommand
{
public:
	CCmdPacketGenSetPacketRate(CWifiTestMgr* pCWifiTestMgr,
					 CTxPacketSettings* pTxPacketSettings,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
													m_pTxPacketSettings(pTxPacketSettings) {}

	virtual ~CCmdPacketGenSetPacketRate() {}

	virtual void WifiAction();

private:

	CTxPacketSettings* m_pTxPacketSettings;

};

class CCmdPacketGenSetPacketPowerLevel : public CWifiCliCommand
{
public:
	CCmdPacketGenSetPacketPowerLevel(CWifiTestMgr* pCWifiTestMgr,
									 CTxPacketSettings* pTxPacketSettings,
									 const std::string & name,
									 const std::string & description,
									 uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
													m_pTxPacketSettings(pTxPacketSettings) {}

	virtual ~CCmdPacketGenSetPacketPowerLevel() {}

	virtual void WifiAction();

private:

	CTxPacketSettings* m_pTxPacketSettings;

};

class CCmdPacketGenSetPacketFillType : public CWifiCliCommand
{
public:
	CCmdPacketGenSetPacketFillType(CWifiTestMgr* pCWifiTestMgr,
									 CTxPacketSettings* pTxPacketSettings,
									 const std::string & name,
									 const std::string & description,
									 uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
													m_pTxPacketSettings(pTxPacketSettings) {}

	virtual ~CCmdPacketGenSetPacketFillType() {}

	virtual void WifiAction();

private:

	CTxPacketSettings* m_pTxPacketSettings;

};


class CCmdPacketGenGenerate : public CWifiCliCommand
{
public:
	CCmdPacketGenGenerate(CWifiTestMgr* pCWifiTestMgr,
					 CTxPacketSettings* pTxPacketSettings,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
									  m_pTxPacketSettings(pTxPacketSettings) {}

	virtual ~CCmdPacketGenGenerate() {}

	virtual void WifiAction();

private:

	CTxPacketSettings* m_pTxPacketSettings;

};



class CCmdPacketGenStop : public CWifiCliCommand
{
public:
	CCmdPacketGenStop(CWifiTestMgr* pCWifiTestMgr,
					 CTxPacketSettings* pTxPacketSettings,
						const std::string & name,
						const std::string & description,
						uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pTxPacketSettings(pTxPacketSettings) {}

	virtual ~CCmdPacketGenStop() {}

	virtual void WifiAction();

private:

	CTxPacketSettings* m_pTxPacketSettings;

};


class CCmdReceivePacketsGet : public CWifiCliCommand
{
public:
	CCmdReceivePacketsGet(CWifiTestMgr* pCWifiTestMgr,
						  CRxSettings* pRxSettings,
						  const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pRxSettings(pRxSettings) {}

	virtual ~CCmdReceivePacketsGet() {}

	virtual void WifiAction();

private:

	CRxSettings* m_pRxSettings;

};


class CCmdReceivePacketsSetMacFilter : public CWifiCliCommand
{
public:
	CCmdReceivePacketsSetMacFilter(CWifiTestMgr* pCWifiTestMgr,
						  CRxSettings* pRxSettings,
						  const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pRxSettings(pRxSettings) {}

	virtual ~CCmdReceivePacketsSetMacFilter() {}

	virtual void WifiAction();

private:

	CRxSettings* m_pRxSettings;

};


class CCmdReceivePacketsUnSetMacFilter : public CWifiCliCommand
{
public:
	CCmdReceivePacketsUnSetMacFilter(CWifiTestMgr* pCWifiTestMgr,
						  CRxSettings* pRxSettings,
						  const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pRxSettings(pRxSettings) {}

	virtual ~CCmdReceivePacketsUnSetMacFilter() {}

	virtual void WifiAction();

private:

	CRxSettings* m_pRxSettings;

};


class CCmdReceivePacketsSetTimeout : public CWifiCliCommand
{
public:
	CCmdReceivePacketsSetTimeout(CWifiTestMgr* pCWifiTestMgr,
						  CRxSettings* pRxSettings,
						  const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pRxSettings(pRxSettings) {}

	virtual ~CCmdReceivePacketsSetTimeout() {}

	virtual void WifiAction();

private:

	CRxSettings* m_pRxSettings;

};


class CCmdReceivePacketsStart : public CWifiCliCommand
{
public:
	CCmdReceivePacketsStart(CWifiTestMgr* pCWifiTestMgr,
						  CRxSettings* pRxSettings,
						  const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pRxSettings(pRxSettings) {}

	virtual ~CCmdReceivePacketsStart() {}

	virtual void WifiAction();

private:

	CRxSettings* m_pRxSettings;

};



class CCmdReceivePacketsStop : public CWifiCliCommand
{
public:
	CCmdReceivePacketsStop(CWifiTestMgr* pCWifiTestMgr,
						  CRxSettings* pRxSettings,
						  const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut),
										m_pRxSettings(pRxSettings) {}

	virtual ~CCmdReceivePacketsStop() {}

	virtual void WifiAction();

private:

	CRxSettings* m_pRxSettings;

};

class CCmdCWTranmissionEnable : public CWifiCliCommand
{
public:
	CCmdCWTranmissionEnable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdCWTranmissionEnable() {}


	virtual void WifiAction();

};


class CCmdCWTranmissionDisable : public CWifiCliCommand
{
public:
	CCmdCWTranmissionDisable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdCWTranmissionDisable() {}


	virtual void WifiAction();

};


class CCmdRSSIGet : public CWifiCliCommand
{
public:
	CCmdRSSIGet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRSSIGet() {}


	virtual void WifiAction();

};


class CCmdRSSISetBeaconWndSize : public CWifiCliCommand
{
public:
	CCmdRSSISetBeaconWndSize(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRSSISetBeaconWndSize() {}


	virtual void WifiAction();

};



class CCmdRSSISetDataWndSize : public CWifiCliCommand
{
public:
	CCmdRSSISetDataWndSize(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRSSISetDataWndSize() {}


	virtual void WifiAction();

};


class CCmdRSSIData : public CWifiCliCommand
{
public:
	CCmdRSSIData(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRSSIData() {}


	virtual void WifiAction();

};


class CCmdRSSIBeacon : public CWifiCliCommand
{
public:
	CCmdRSSIBeacon(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRSSIBeacon() {}


	virtual void WifiAction();

};


class CCmdRegistersWrite : public CWifiCliCommand
{
public:
	CCmdRegistersWrite(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersWrite() {}


	virtual void WifiAction();

};

class CCmdRegistersWriteField : public CWifiCliCommand
{
public:
	CCmdRegistersWriteField(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersWriteField() {}


	virtual void WifiAction();

};

class CCmdRegistersWriteBit : public CWifiCliCommand
{
public:
	CCmdRegistersWriteBit(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersWriteBit() {}


	virtual void WifiAction();

};


class CCmdRegistersRead : public CWifiCliCommand
{
public:
	CCmdRegistersRead(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersRead() {}


	virtual void WifiAction();

};

class CCmdRegistersReadField : public CWifiCliCommand
{
public:
	CCmdRegistersReadField(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersReadField() {}


	virtual void WifiAction();

};

class CCmdRegistersReadBit : public CWifiCliCommand
{
public:
	CCmdRegistersReadBit(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersReadBit() {}


	virtual void WifiAction();

};


class CCmdRegistersSave : public CWifiCliCommand
{
public:
	CCmdRegistersSave(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersSave() {}


	virtual void WifiAction();

};


class CCmdRegistersLoad : public CWifiCliCommand
{
public:
	CCmdRegistersLoad(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersLoad() {}


	virtual void WifiAction();

};


class CCmdRegistersUpload : public CWifiCliCommand
{
public:
	CCmdRegistersUpload(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersUpload() {}


	virtual void WifiAction();

};


class CCmdRegistersDownload : public CWifiCliCommand
{
public:
	CCmdRegistersDownload(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRegistersDownload() {}


	virtual void WifiAction();

};


class CCmdDFSChannelGet : public CWifiCliCommand
{
public:
	CCmdDFSChannelGet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDFSChannelGet() {}


	virtual void WifiAction();

};



class CCmdDFSChannelSet : public CWifiCliCommand
{
public:
	CCmdDFSChannelSet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDFSChannelSet() {}


	virtual void WifiAction();

};



class CCmdDFSEventGet : public CWifiCliCommand
{
public:
	CCmdDFSEventGet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDFSEventGet() {}


	virtual void WifiAction();

};


class CCmdDFSEventEnable : public CWifiCliCommand
{
public:
	CCmdDFSEventEnable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDFSEventEnable() {}


	virtual void WifiAction();

};


class CCmdDFSEventDisable : public CWifiCliCommand
{
public:
	CCmdDFSEventDisable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDFSEventDisable() {}


	virtual void WifiAction();

};


class CCmdPowerSaveGet : public CWifiCliCommand
{
public:
	CCmdPowerSaveGet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveGet() {}


	virtual void WifiAction();

};

class CCmdPowerSaveEnable : public CWifiCliCommand
{
public:
	CCmdPowerSaveEnable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveEnable() {}


	virtual void WifiAction();

};

class CCmdPowerSaveDisable : public CWifiCliCommand
{
public:
	CCmdPowerSaveDisable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveDisable() {}


	virtual void WifiAction();

};

class CCmdPowerSaveSetAction : public CWifiCliCommand
{
public:
	CCmdPowerSaveSetAction(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveSetAction() {}


	virtual void WifiAction();

};

class CCmdPowerSaveSetAcMode : public CWifiCliCommand
{
public:
	CCmdPowerSaveSetAcMode(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveSetAcMode() {}


	virtual void WifiAction();

};
#ifndef ANDROID
class CCmdPowerSaveTriggerFrameStart : public CWifiCliCommand
{
public:
	CCmdPowerSaveTriggerFrameStart(CWifiTestMgr* pCWifiTestMgr,
									const std::string & name,
									const std::string & description,
								   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
																   name,
																   description,
																   shortcut) {}

	virtual ~CCmdPowerSaveTriggerFrameStart() {}


	virtual void WifiAction();

};


class CCmdPowerSaveTriggerFrameStop : public CWifiCliCommand
{
public:
	CCmdPowerSaveTriggerFrameStop(CWifiTestMgr* pCWifiTestMgr,
									const std::string & name,
									const std::string & description,
								   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
																   name,
																   description,
																   shortcut) {}

	virtual ~CCmdPowerSaveTriggerFrameStop() {}


	virtual void WifiAction();

};
#endif //ANDROID

/*
class CCmdPowerSaveSetAlwaysOn : public CWifiCliCommand
{
public:
	CCmdPowerSaveSetAlwaysOn(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveSetAlwaysOn() {}


	virtual void WifiAction();

};


class CCmdPowerSaveSetDoze : public CWifiCliCommand
{
public:
	CCmdPowerSaveSetDoze(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveSetDoze() {}


	virtual void WifiAction();

};


class CCmdPowerSaveSetDeepSleep : public CWifiCliCommand
{
public:
	CCmdPowerSaveSetDeepSleep(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdPowerSaveSetDeepSleep() {}


	virtual void WifiAction();

};

*/
class CCmdNuisanceGet : public CWifiCliCommand
{
public:
	CCmdNuisanceGet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdNuisanceGet() {}


	virtual void WifiAction();

};


class CCmdNuisanceEnable : public CWifiCliCommand
{
public:
	CCmdNuisanceEnable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdNuisanceEnable() {}


	virtual void WifiAction();

};


class CCmdNuisanceDisable : public CWifiCliCommand
{
public:
	CCmdNuisanceDisable(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdNuisanceDisable() {}


	virtual void WifiAction();

};


class CCmdDiversityGet : public CWifiCliCommand
{
public:
	CCmdDiversityGet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDiversityGet() {}


	virtual void WifiAction();

};


class CCmdDiversityMRC : public CWifiCliCommand
{
public:
	CCmdDiversityMRC(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDiversityMRC() {}


	virtual void WifiAction();

};

class CCmdDiversityAntennaA : public CWifiCliCommand
{
public:
	CCmdDiversityAntennaA(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDiversityAntennaA() {}


	virtual void WifiAction();

};


class CCmdDiversityAntennaB : public CWifiCliCommand
{
public:
	CCmdDiversityAntennaB(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDiversityAntennaB() {}


	virtual void WifiAction();

};


class CCmdMib32Get : public CWifiCliCommand
{
public:
	CCmdMib32Get(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMib32Get() {}


	virtual void WifiAction();

};



class CCmdMib32Set : public CWifiCliCommand
{
public:
	CCmdMib32Set(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMib32Set() {}


	virtual void WifiAction();

};

class CCmdMibMacAddrGet : public CWifiCliCommand
{
public:
	CCmdMibMacAddrGet(CWifiTestMgr* pCWifiTestMgr,
					  const std::string & name,
					  const std::string & description,
					  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMibMacAddrGet() {}


	virtual void WifiAction();

};



class CCmdMibMacAddrSet : public CWifiCliCommand
{
public:
	CCmdMibMacAddrSet(CWifiTestMgr* pCWifiTestMgr,
					  const std::string & name,
					  const std::string & description,
					  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMibMacAddrSet() {}


	virtual void WifiAction();

};


class CCmdMibSsidGet : public CWifiCliCommand
{
public:
	CCmdMibSsidGet(CWifiTestMgr* pCWifiTestMgr,
					  const std::string & name,
					  const std::string & description,
					  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMibSsidGet() {}


	virtual void WifiAction();

};



class CCmdMibSsidSet : public CWifiCliCommand
{
public:
	CCmdMibSsidSet(CWifiTestMgr* pCWifiTestMgr,
					  const std::string & name,
					  const std::string & description,
					  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdMibSsidSet() {}


	virtual void WifiAction();

};



class CCmdDriverModeGet : public CWifiCliCommand
{
public:
	CCmdDriverModeGet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDriverModeGet() {}


	virtual void WifiAction();

};


class CCmdDriverModeSet : public CWifiCliCommand
{
public:
	CCmdDriverModeSet(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDriverModeSet() {}


	virtual void WifiAction();

};


class CCmdDriverStart : public CWifiCliCommand
{
public:
	CCmdDriverStart(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDriverStart() {}


	virtual void WifiAction();

};




class CCmdDriverStop : public CWifiCliCommand
{
public:
	CCmdDriverStop(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDriverStop() {}


	virtual void WifiAction();

};



class CCmdRFTestTransmit : public CWifiCliCommand
{
public:
	CCmdRFTestTransmit(CWifiTestMgr* pCWifiTestMgr,
					   const std::string & name,
					   const std::string & description,
					   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													 name,
													 description,
													 shortcut) {}

	virtual ~CCmdRFTestTransmit() {}


	virtual void WifiAction();

};

class CCmdRFTestTransmitEnhanced : public CWifiCliCommand
{
public:
	CCmdRFTestTransmitEnhanced(CWifiTestMgr* pCWifiTestMgr,
					   const std::string & name,
					   const std::string & description,
					   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													 name,
													 description,
													 shortcut) {}

	virtual ~CCmdRFTestTransmitEnhanced() {}


	virtual void WifiAction();

};



class CCmdRFTestReceive : public CWifiCliCommand
{
public:
	CCmdRFTestReceive(CWifiTestMgr* pCWifiTestMgr,
					   const std::string & name,
					   const std::string & description,
					   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													 name,
													 description,
													 shortcut) {}

	virtual ~CCmdRFTestReceive() {}


	virtual void WifiAction();

};

class CCmdRFTestReceiveEnhanced : public CWifiCliCommand
{
public:
	CCmdRFTestReceiveEnhanced(CWifiTestMgr* pCWifiTestMgr,
					   const std::string & name,
					   const std::string & description,
					   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													 name,
													 description,
													 shortcut) {}

	virtual ~CCmdRFTestReceiveEnhanced() {}


	virtual void WifiAction();

};



class CCmdRFTestRxGainCalibration : public CWifiCliCommand
{
public:
	CCmdRFTestRxGainCalibration(CWifiTestMgr* pCWifiTestMgr,
								   const std::string & name,
								   const std::string & description,
								   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
																 name,
																 description,
																 shortcut) {}

	virtual ~CCmdRFTestRxGainCalibration() {}


	virtual void WifiAction();

private:

	EWifiTestStatus InitStage(bool *pIsExternalLNA);


	EWifiTestStatus ReadRegisters(uint8 *pPwr100dBm,
								  uint8 *pInitAGain,
								  uint8 *pStepLNA,
								  uint8 *pRssiDb);

	EWifiTestStatus SetRegisters(uint8 lnaGain,
								 uint8 lnaAtten,
								 uint8 initAGain);

	EWifiTestStatus SaveRegisters(uint8 newInitAGain,
								  uint8 newStepLNA);


	EWifiTestStatus GetReceivedRSSI(double *pAvgRssiA,
									double *pAvgRssiB);

	void CalculateNewValues(double dRSSI0_1,
							double dRSSI1_1,
							double dRSSI0_0,
							double dRSSI1_0,
							double multFactor,
							uint8 initAGain,
							uint8 stepLNA,
							uint8  pwr100dBm,
							uint8 *pNewInitAGain,
							uint8 *pNewStepLNA);
private:

	CRxSettings m_RxSettings;

};


class CCmdRFTestTransmitStop : public CWifiCliCommand
{
public:
	CCmdRFTestTransmitStop(CWifiTestMgr* pCWifiTestMgr,
                           const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
														 name,
														 description,
														 shortcut) {}

	virtual ~CCmdRFTestTransmitStop() {}

	virtual void WifiAction();

};


class CCmdRFTestReceiveStop : public CWifiCliCommand
{
public:
	CCmdRFTestReceiveStop(CWifiTestMgr* pCWifiTestMgr,
                          const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
														name,
														description,
														shortcut)  {}

	virtual ~CCmdRFTestReceiveStop() {}

	virtual void WifiAction();
};


class CCmdRFTestRateMask : public CWifiCliCommand
{
public:
	CCmdRFTestRateMask(CWifiTestMgr* pCWifiTestMgr,
                          const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
														name,
														description,
														shortcut)  {}

	virtual ~CCmdRFTestRateMask() {}

	virtual void WifiAction();
};

class CCmdSysSettingGetIntegerFromFile : public CWifiCliCommand
{
public:
	CCmdSysSettingGetIntegerFromFile(CWifiTestMgr* pCWifiTestMgr,
							 const std::string & name,
							 const std::string & description,
							 uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
															 name,
															 description,
															 shortcut)  {}

	virtual ~CCmdSysSettingGetIntegerFromFile() {}

	virtual void WifiAction();

protected:

	virtual void GetIntegerFromFile(const char* FileName = NULL);

};


class CCmdSysSettingSetIntegerInFile : public CWifiCliCommand
{
public:
	CCmdSysSettingSetIntegerInFile(CWifiTestMgr* pCWifiTestMgr,
							 const std::string & name,
							 const std::string & description,
							 uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
															 name,
															 description,
															 shortcut)  {}

	virtual ~CCmdSysSettingSetIntegerInFile() {}

	virtual void WifiAction();

protected:

	virtual void SetIntegerInFile(const char* FileName = NULL);

};

class CCmdSysSettingGetStringFromFile : public CWifiCliCommand
{
public:
	CCmdSysSettingGetStringFromFile(CWifiTestMgr* pCWifiTestMgr,
							const std::string & name,
							const std::string & description,
							uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
															name,
															description,
															shortcut)  {}

	virtual ~CCmdSysSettingGetStringFromFile() {}

	virtual void WifiAction();

protected:

	virtual void GetStringFromFile(const char* FileName = NULL);

};


class CCmdSysSettingSetStringInFile : public CWifiCliCommand
{
public:
	CCmdSysSettingSetStringInFile(CWifiTestMgr* pCWifiTestMgr,
							const std::string & name,
							const std::string & description,
							uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
															name,
															description,
															shortcut)  {}

	virtual ~CCmdSysSettingSetStringInFile() {}

	virtual void WifiAction();

protected:

	virtual void SetStringInFile(const char* FileName = NULL);

};


class CCmdSysSettingGetBoardId : public CWifiCliCommand
{
public:
	CCmdSysSettingGetBoardId(CWifiTestMgr* pCWifiTestMgr,
							const std::string & name,
							const std::string & description,
							uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
															name,
															description,
															shortcut)  {}

	virtual ~CCmdSysSettingGetBoardId() {}

	virtual void WifiAction();

};

class CCmdSysSettingGetInteger : public CCmdSysSettingGetIntegerFromFile
{
public:
	CCmdSysSettingGetInteger(CWifiTestMgr* pCWifiTestMgr,
							 const std::string & name,
							 const std::string & description,
							 uint8 shortcut):CCmdSysSettingGetIntegerFromFile(pCWifiTestMgr,
															 name,
															 description,
															 shortcut)  {}

	virtual ~CCmdSysSettingGetInteger() {}

	virtual void WifiAction();
};


class CCmdSysSettingSetInteger : public CCmdSysSettingSetIntegerInFile
{
public:
	CCmdSysSettingSetInteger(CWifiTestMgr* pCWifiTestMgr,
							 const std::string & name,
							 const std::string & description,
							 uint8 shortcut):CCmdSysSettingSetIntegerInFile(pCWifiTestMgr,
															 name,
															 description,
															 shortcut)  {}

	virtual ~CCmdSysSettingSetInteger() {}

	virtual void WifiAction();
};

class CCmdSysSettingGetString : public CCmdSysSettingGetStringFromFile
{
public:
	CCmdSysSettingGetString(CWifiTestMgr* pCWifiTestMgr,
							const std::string & name,
							const std::string & description,
							uint8 shortcut):CCmdSysSettingGetStringFromFile(pCWifiTestMgr,
															name,
															description,
															shortcut)  {}

	virtual ~CCmdSysSettingGetString() {}

	virtual void WifiAction();
};


class CCmdSysSettingSetString : public CCmdSysSettingSetStringInFile
{
public:
	CCmdSysSettingSetString(CWifiTestMgr* pCWifiTestMgr,
							const std::string & name,
							const std::string & description,
							uint8 shortcut):CCmdSysSettingSetStringInFile(pCWifiTestMgr,
															name,
															description,
															shortcut)  {}

	virtual ~CCmdSysSettingSetString() {}

	virtual void WifiAction();
};


class CCmdSysSettingSyncInteger : public CWifiCliCommand
{
public:
	CCmdSysSettingSyncInteger(CWifiTestMgr* pCWifiTestMgr,
                          const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
														name,
														description,
														shortcut)  {}

	virtual ~CCmdSysSettingSyncInteger() {}

	virtual void WifiAction();
};

class CCmdSysSettingGetBoardRfParam : public CWifiCliCommand
{
public:
	CCmdSysSettingGetBoardRfParam(CWifiTestMgr* pCWifiTestMgr,
							const std::string & name,
							const std::string & description,
							uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
															name,
															description,
															shortcut)  {}

	virtual ~CCmdSysSettingGetBoardRfParam() {}

	virtual void WifiAction();

};



class CCmdRfCalibration : public CWifiCliCommand
{
public:

	CCmdRfCalibration(CWifiTestMgr* pCWifiTestMgr,
                          const std::string & name,
						  const std::string & description,
						  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
														name,
														description,
														shortcut)	{}

	virtual ~CCmdRfCalibration() {}

	virtual void WifiAction();

private:

	char* GetValue(const char* group, const char *key);

	int ParseLine(vector<uint32>& valVector,
				  const std::string& field);

	int ConvertField(const std::string& field,uint32* pCurVal);

	int LoadGroup(IN const char* strGroupName,
				  IN EWmMibPhyParameterType type,
				  IN uint32 validEntryLength,
				  OUT WmMibPhyParameter &stPhyParam);

	void LoadDataForDebug();
};


class CCmdDBGMsgGetInteger : public CWifiCliCommand
{
public:
	CCmdDBGMsgGetInteger(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDBGMsgGetInteger() {}


	virtual void WifiAction();

};

class CCmdDBGMsgSetInteger : public CWifiCliCommand
{
public:
	CCmdDBGMsgSetInteger(CWifiTestMgr* pCWifiTestMgr,
						   const std::string & name,
						   const std::string & description,
						   uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdDBGMsgSetInteger() {}


	virtual void WifiAction();

};


class CCmdRFTestMonitor : public CWifiCliCommand
{
public:
	CCmdRFTestMonitor(CWifiTestMgr* pCWifiTestMgr,
					  const std::string & name,
					  const std::string & description,
					  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRFTestMonitor() {}


	virtual void WifiAction();

};


class CCmdRFTestMonitorControl : public CWifiCliCommand
{
public:
	CCmdRFTestMonitorControl(CWifiTestMgr* pCWifiTestMgr,
					  const std::string & name,
					  const std::string & description,
					  uint8 shortcut):CWifiCliCommand(pCWifiTestMgr,
													  name,
													  description,
													  shortcut) {}

	virtual ~CCmdRFTestMonitorControl() {}


	virtual void WifiAction();

};
	



#endif //MENUCOMMANDS_H

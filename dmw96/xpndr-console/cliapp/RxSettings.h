#ifndef RXSETTINGS_H
#define RXSETTINGS_H

#include "TypeDefs.h"
#include "WifiMgr/include/WifiTestMgr.h"


class CRxSettings
{
public:

	CRxSettings();

	~CRxSettings();

	void DisplayRxSettings();

	void static DisplayRxResults(CWifiTestMgrRecvResults* pRecvRes);


	WmMacAddress MacFilter;
	uint32 iTimeout;
	bool isMacFilterSet;

};

#endif //RXSETTINGS_H

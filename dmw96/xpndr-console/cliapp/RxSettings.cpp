#include "RxSettings.h"
#include <iostream>
#include <iomanip>
using namespace std;


CRxSettings::CRxSettings()
{
    iTimeout = 0;
	isMacFilterSet = false;
}

CRxSettings::~CRxSettings()
{

}

void CRxSettings::DisplayRxSettings()
{

	cout<<endl<<endl<<"Current Receive Settings"<<endl;


	cout<<setw(20)<<left<<"Receive Timeout: "<<iTimeout<<endl;

	if(isMacFilterSet) {

		cout<<setw(20)<<left<<"Mac Filtering: ";

		for (int byteAddr=0;byteAddr<5;byteAddr++) {

			cout<<std::hex<<(uint32)MacFilter.Address[byteAddr]<<":";

		}

		cout<<std::hex<<(uint32)MacFilter.Address[5]<<endl;

		}

	else
	{
		cout<<setw(20)<<left<<"Mac Filtering: "<<"OFF"<<endl;
	}

	cout<<std::dec;

}

void CRxSettings::DisplayRxResults(CWifiTestMgrRecvResults* pRecvRes)
{

	if(pRecvRes == NULL)
	{
		return;
	}

	cout<<"Total Packets Received\t " << pRecvRes->totalPacketsReceived << endl;
	cout<<"Filtered Packets Received\t " << pRecvRes->filteredPacketsReceived << endl;
	cout<<"Total Received Without Error\t " << pRecvRes->totalReceivedWithoutError << endl;
	cout<<"Filtered Received Without Error\t " << pRecvRes->filteredReceivedWithoutError << endl;

    cout<<"Average RSSI A of Packets Received Without Error\t " << pRecvRes->avgRSSI_A - 256.0 + 100.0 << endl;
	cout<<"Average RSSI B of Packets Received Without Error\t " << pRecvRes->avgRSSI_B - 256.0 + 100.0 << endl;


	float dTotErrRate,dFiltErrRate;

	if(pRecvRes->totalPacketsReceived == 0) {

			dTotErrRate = 1.0;

	}
	else
	{
			dTotErrRate = (float) (1.0 - ( (double)(pRecvRes->totalReceivedWithoutError) / (double)(pRecvRes->totalPacketsReceived) ) );
	}

//		cout<<"Total Error Rate\t"<<dTotErrRate<< endl;
		

	if(pRecvRes->filteredPacketsReceived == 0) {

			dFiltErrRate = 1.0;
	}
	else
	{
			dFiltErrRate = (float) (1.0 - ( (double)(pRecvRes->filteredReceivedWithoutError) / (double)(pRecvRes->filteredPacketsReceived) ) );
	}

	//	cout<<"Filtered Error Rate\t"<<dFiltErrRate << endl;
}


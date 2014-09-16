//////////////////////////////////////////////////////////////////////////////
// Includes.
// 
#include "WifiMgr/include/WifiTestMgr.h"
#ifndef ANDROID
#include "infra/include/InfraMacros.h"
#include "infra/include/InfraLog.h"
#include "infra/include/InfraMutex.h"
#include "infra/include/InfraGuard.h"
#endif //ANDROID

#define NUM_OF_MEM_POOLS	(0)
#define MAX_FILE_NAME 512
#define MAX_LOGGER_TARGET_LEN 256
#define MAX_LOGGER_STRING 512

#ifndef ANDROID
#define LOG_DESTINATION  	 "CONSOLE"
#define LOG_DEF_DESTINATION  "NET:127.0.0.1:7001"
#endif //ANDROID


#include <stdio.h>
#include "XpndrCli.h"	


// Version
#ifndef XPNDR_CLI_VERSION
#define XPNDR_CLI_VERSION "UNKNOWN"
#endif


void DisplayUsage()
{
	printf("XpndrCli [--s] [--l loggeraddr] [--f filename] [--p arglist]\n"
		   "--s:\t\tSilent mode\n"
		   "--l loggeraddr:\t\tLogger debug output redirection. "
		   " Format NET:172.19.35.50:7001 or CONSOLE\n"
           "--f filename:\tRead input from file\n"
		   "--p arglist:\t\tExecute commands from arglist and exit\n"
		   );
}

int ParseInputFileCommand(int argc,
						  char* argv[],
						  int* pCurArgcInd,
                          bool* pReadFromFile,
						  char* pFileName,
						  uint32 maxFileLen)
{
	ASSERT(pCurArgcInd);

	if(*pCurArgcInd > argc - 2)
	{
		return 0;
	}

	if(!strcmp(argv[*pCurArgcInd],"--f"))
	{
		if(pFileName == NULL || maxFileLen == 0)
		{
			return -2;
		}

		strncpy(pFileName,argv[*pCurArgcInd + 1],maxFileLen);

		*pReadFromFile = true;

		*pCurArgcInd = *pCurArgcInd + 2;

	}

	return 0;

}

int ParseLogCommand(int argc,
					char* argv[],
					int* pCurArgcInd,
                    bool* pRedirectLogger,
					char* pLoggerTarget,
					uint32 maxLoggerNameLen)
{
	ASSERT(pCurArgcInd);

	if(*pCurArgcInd > argc - 2)
	{
		return 0;
	}

	if(!strcmp(argv[*pCurArgcInd],"--l"))
	{
		if(pLoggerTarget == NULL || maxLoggerNameLen == 0)
		{
			return -2;
		}

		strncpy(pLoggerTarget,argv[*pCurArgcInd + 1],maxLoggerNameLen);

		*pRedirectLogger = true;

		*pCurArgcInd = *pCurArgcInd + 2;

	}

	return 0;

}

int ParseInputFileStream(int argc,
						 char* argv[],
						 int* pCurArgcInd,
                         bool* pReadFromStream)
{
	ASSERT(pCurArgcInd);

	if(*pCurArgcInd > argc - 2)
	{
		return 0;
	}

	if(!strcmp(argv[*pCurArgcInd],"--p"))
	{
		*pReadFromStream = true;

	}

	return 0;

}


int ParseCommandLineArguments(int argc,
							  char* argv[],
							  bool* pDisplaySilent,
							  bool* pRedirectLogger,
							  char* pLoggerTarget,
							  uint32 maxLoggerNameLen,
							  bool* pReadFromFile,
							  char* pFileName,
							  uint32 maxFileLen,
							  bool* pReadFromStream,
							  uint8* pFirstStreamArgInd)
{

	*pDisplaySilent = false;
	*pReadFromFile 	= false;
	*pReadFromStream = false;
	*pRedirectLogger = false;

	int curArgcInd = 1;

	if(argc == 1)
	{
		return 0;
	}

	if(!strcmp(argv[curArgcInd],"--s"))
	{
		*pDisplaySilent = true;

		curArgcInd++;

	}



	if(ParseLogCommand(argc,
					   argv,
					   &curArgcInd,
					   pRedirectLogger,
					   pLoggerTarget,
					   maxLoggerNameLen) < 0)
	{
		return -1;
	}

	if(ParseInputFileCommand(argc,
							 argv,
							 &curArgcInd,
							 pReadFromFile,
							 pFileName,
							 maxFileLen) < 0)
	{
		return -1;
	}

	if(*pReadFromFile)
	{
		return 0;
	}

	if(ParseInputFileStream(argc,
							argv,
							&curArgcInd,
							pReadFromStream) < 0)
	{

		return -1;

	}

	if(*pReadFromStream)
	{
		*pFirstStreamArgInd = curArgcInd + 1;
	}

	return 0;
}


int main(int argc,char* argv[])
{

	// Initialize the OS-Wrapper 
#ifndef ANDROID
	OSL_Init();
#endif //ANDROID

	bool bDisplaySilent = false;
	bool bReadFromFile = false;
	bool bReadFromStream = false;
	char fileName[MAX_FILE_NAME];
	uint8 firstStreamArgInd = 0;

	bool bRedirectLogger = false;
	char loggerTarget[MAX_LOGGER_TARGET_LEN];

	int iRes = ParseCommandLineArguments(argc,
										 argv,
										 &bDisplaySilent,
										 &bRedirectLogger,
										 loggerTarget,
										 MAX_LOGGER_TARGET_LEN,
										 &bReadFromFile,
										 fileName,
										 MAX_FILE_NAME,
										 &bReadFromStream,
										 &firstStreamArgInd);

	printf("\n%s v%s (built %s %s)\n",	
		argv[0], 
		XPNDR_CLI_VERSION, 
		__DATE__, 
		__TIME__);


	if(iRes < 0)
	{
		printf("Error: Bad command line arguments\n");

		DisplayUsage();

		return 4;

	}

	//logs configurations
#ifndef ANDROID
	char LogConfig [MAX_LOGGER_STRING];

	if(bRedirectLogger) {

		sprintf(LogConfig,
				"[0]\r\nNAME=DEF\r\nLEVEL=28\r\nOUTPUT=%s\r\n"
				"[38]\r\nNAME=WIFI\r\nLEVEL=7\r\nOUTPUT=%s\r\n",
				LOG_DEF_DESTINATION,
				loggerTarget);

    }
	else
	{
		sprintf(LogConfig,
				"[0]\r\nNAME=DEF\r\nLEVEL=28\r\nOUTPUT=%s\r\n"
				"[38]\r\nNAME=WIFI\r\nLEVEL=7\r\nOUTPUT=%s\r\n",
				LOG_DEF_DESTINATION,
				LOG_DEF_DESTINATION);

	}


   
	InfraLogConfigure(LogConfig,0,0,'\r');
#endif //ANDROID

	CWifiTestMgr* pWifiTestMgr = CWifiTestMgr::GetInstance();

	if(!pWifiTestMgr) {

		printf("\nError: Cannot create instance of WifiTestMgr\n");

		return 1;

	}

	bool bRes = pWifiTestMgr->Init();

   if(bRes == false) {

	   printf("\nError: Cannot initialize of WifiTestMgr\n");

	   return 2;

   }


   CXpndrWifiCli* pWifiCli;

	if(bDisplaySilent)
	{
		printf("\nRunning in silent mode\n");
    }

	if(bReadFromFile)
	{
		printf("Reading input from %s\n",fileName);

		pWifiCli = new CXpndrWifiCli(pWifiTestMgr,
                                     fileName,
									 bDisplaySilent);
	}
	else if(bReadFromStream)
	{
		printf("Reading input from command line stream , num of args is %d\n",argc - firstStreamArgInd);

		pWifiCli = new CXpndrWifiCli(pWifiTestMgr,
                                    &(argv[firstStreamArgInd]),
									 argc - firstStreamArgInd,
									 bDisplaySilent);


	}
	else
	{
		pWifiCli = new CXpndrWifiCli(pWifiTestMgr,
									 bDisplaySilent);

	}

	if(pWifiCli == NULL)
	{
		printf("\nError: Cannot build menu\n");

		return 2;
	}

	iRes = pWifiCli->Run();

	if(iRes < 0) 
	{
		printf("\nError: Cannot start menu.Error code %d\n",iRes);

		return 3;

	}

	bool isReceiverActive = false;

	pWifiTestMgr->GetReceiveStatus(&isReceiverActive);


	if(isReceiverActive)
	{
		printf("\nReceiver proc is still active, waiting for stop or exit command\n");

		iRes = pWifiCli->Run();

		if(iRes < 0) 
		{
			printf("\nError: Cannot start menu.Error code %d\n",iRes);

			return 4;

		}

		printf("\nDone\n");

	}


	delete pWifiCli;

	pWifiTestMgr->Terminate();

	return 0;
}



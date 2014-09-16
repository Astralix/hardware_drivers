//--< wiMain.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  Entry point for execution in WiSE
//  Copyright 2001-2004 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wiMain.h"
#include "wiMatrix.h"
#include "wiParse.h"
#include "wiPHY.h"
#include "wiRnd.h"
#include "wiMAC.h"
#include "wiTest.h"
#include "wise.h"

static wiBoolean WiseIsInitialized = 0;

#define  STATUS(x)  WICHK(x)
#define XSTATUS(x)  if (WICHK(x) < 0) return WI_ERROR_RETURNED;

// ================================================================================================
// FUNCTION  : wiMain()
// ------------------------------------------------------------------------------------------------
// Purpose   : Entry point for program execution
// Parameters: argc, argv -- command line arguments passed from main()
// ================================================================================================
int wiMain(int argc, char *argv[])
{
    time_t t = time(0);
    char script_filename[384] = {"wise.txt"};
    int i;

    wiBoolean ResumePausedScript = WI_FALSE;
    wiBoolean NoHeader           = WI_FALSE;
    wiBoolean PrintHelp          = WI_FALSE;
    wiBoolean ParseOptions       = WI_FALSE;

    // -------------------------------
    // Get the command line arguments
    // ------------------------------
    for (i = 1; i < argc; i++)
    {
             if (!strcmp(argv[i],"-Help"))      PrintHelp = WI_TRUE;
        else if (!strcmp(argv[i],"-Trace"))     { XSTATUS(wiMemory_EnableTraceMemoryManagement()) }
        else if (!strcmp(argv[i],"-AddrLimit")) { XSTATUS(wiMemory_EnabledMemoryAddressLimit()) }
        else if (!strcmp(argv[i],"-Resume"))    ResumePausedScript = 1;
        else if (!strcmp(argv[i],"-NoHeader"))  NoHeader = 1;
        else if (!strcmp(argv[i],"-p")) 
        {
            if (i+1 < argc) {
                ParseOptions = 1;
                i++;
            }
            else PrintHelp = 1;
        }
        else if (argv[i][0] == '-') PrintHelp = WI_TRUE;
        else if (argv[i][0] == '/') PrintHelp = WI_TRUE;
        else 
        {
            if (i == (argc-1))
                sprintf(script_filename, "%s", argv[i]);
            else
                PrintHelp = WI_TRUE;
        }
    }
    // --------------------------------------------------------------------
    // Print the title banner if wiMain is called as main(); e.g., argc > 0
    // --------------------------------------------------------------------
    if (argc > 0 && !NoHeader) 
    {
        wiPrintf("\n");
        wiPrintf(" _____________________________________________________________________________\n\n");
        wiPrintf(" %s (%s) -- Wireless Simulation Environment\n",WISE_VERSION_STRING,__DATE__);
        wiPrintf("   Copyright 2001-2004 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.\n");
        wiPrintf(" _____________________________________________________________________________\n\n");
    }
    // --------------------------
    // Display command line usage
    // --------------------------
    if (PrintHelp)
    {
        wiPrintf(" WISE [-Help] [-Trace] [-p <Script Line Text>] [filename]\n");
        wiPrintf("\n");
        wiPrintf("      Run the Wireless Simulation Environment using the script file specified\n");
        wiPrintf("      by [filename]; otherwise the default script 'wise.txt' is used.\n");
        wiPrintf("\n");
        wiPrintf("      -Help         displays this command line usage information\n");
        wiPrintf("      -Resume       continue through pauses in a script file\n");
        wiPrintf("      -Trace        enable memory management trace\n");
        wiPrintf("      -NoHeader     supress the startup title/copyright message\n");
        wiPrintf("      -p <Text>     processes <Text> with the script parser\n");
        wiPrintf("\n");

        return WI_SUCCESS;
    }

    // Open the log file and include a startup timestamp ////////////////////////////////
    // 
    wiUtil_WriteLogFile(
        "-----------------------------------------------------------------------------\n"
        "%s Build %X (%s) starting at %s\n",
        WISE_VERSION_STRING, WISE_BUILD_VERSION, __DATE__, ctime(&t) );


    // Initialize the all of the individual modules  ////////////////////////////////////
    //
    XSTATUS( wiMain_Init() ); 

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Parse command line options and the script file
    //
    //  This is where the simulator action occurs. Optional command line text is handled 
    //  by the parser. Then the script file is called. The default script file name is 
    //  used unless another is specified on the command line. Instructions to perform 
    //  simulations or perform other activities are dispatched from within the parser.
    //
    if (ParseOptions)
    {
        for (i = 1; i < argc; i++) {
            if (!strcmp(argv[i],"-p")) 
               STATUS( wiParse_Line(argv[++i]) );
        }
    }
    STATUS(wiParse_ScriptFile(script_filename));
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  If the "-Resume" option is invoked, resume script processing until it no longer
    //  pauses during processing
    //
    while (ResumePausedScript && wiParse_ScriptIsPaused())
    {
        STATUS( wiParse_ResumeScript() );
    }

    STATUS( wiMain_Close() ); // Close the individual modules

    //  Close the log file and include an ending timestamp  /////////////////////////////
    //
    t = time(0);
    wiUtil_WriteLogFile(
        "%s Build %X (%s) ending at %s"
        "-----------------------------------------------------------------------------\n",
        WISE_VERSION_STRING, WISE_BUILD_VERSION, __DATE__, ctime(&t));
    wiUtil_CloseLogFile();

    return EXIT_SUCCESS;
}
// end of wiMain()

// ================================================================================================
// FUNCTION  : wiMain_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize all WiSE modules
// Parameters: none
// ================================================================================================
wiStatus wiMain_Init(void)
{
    if (WiseIsInitialized) return WI_ERROR_MODULE_ALREADY_INIT;

    XSTATUS( wiUtil_Init()    );
    XSTATUS( wiParse_Init()   );
    XSTATUS( wiMatrix_Init()  );
    XSTATUS( wiTest_Init()    );
    XSTATUS( wiMAC_Init()     );
    XSTATUS( wiChanl_Init()   );
    XSTATUS( wiPHY_Init()     );

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initialize optional components
    //
    //  To allow this file to be used with multiple different builds and initialize only the
    //  included components, compiler defines look for the include definitions from the module
    //  header files. If the definition exists, the initialization occurs. A similar process
    //  occurs in wiMain_Close()
    // 
    #if defined(__DAKOTATEST_H)
        XSTATUS(  DakotaTest_Init() );
    #endif

    #if defined(__MOJAVETEST_H)
        XSTATUS(  MojaveTest_Init() );
    #endif

    #if defined(__PHY11NTEST_H)
        XSTATUS(  Phy11nTest_Init() );
    #endif

    WiseIsInitialized = WI_TRUE;
    return WI_SUCCESS;
}
// end of wiMain_Init()

// ================================================================================================
// FUNCTION  : wiMain_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Close and clean-up all WiSE modules
// Parameters: none
// ================================================================================================
wiStatus wiMain_Close(void)
{
    if (!WiseIsInitialized) return WI_ERROR_MODULE_NOT_INITIALIZED;

    #if defined(__PHY11NTEST_H)
        XSTATUS(  Phy11nTest_Close() );
    #endif

    #if defined(__MOJAVETEST_H)
        XSTATUS(  MojaveTest_Close() );
    #endif

    #if defined(__DAKOTATEST_H)
        XSTATUS(  DakotaTest_Close() );
    #endif

    XSTATUS( wiPHY_Close()    );
    XSTATUS( wiChanl_Close()  );
    XSTATUS( wiMAC_Close()    );
    XSTATUS( wiTest_Close()   );
    XSTATUS( wiMatrix_Close() );
    XSTATUS( wiParse_Close()  );
    XSTATUS( wiUtil_Close()   );

    WiseIsInitialized = WI_FALSE;
    return WI_SUCCESS;
}
// end of wiMain_Close()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

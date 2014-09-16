/*--< wiExternal.c >--------------------------------------------------------------------\
+---------------------------------------------------------------------------------------+
|  Skeleton for external code added to the WiSE build                                   |
|                                                                                       |
|  Developed by Barrett Brickner                                                        |
|  Copyright 2005 DSP Group, Inc. All rights reserved.                                  |
+---------------------------------------------------------------------------------------+
\--------------------------------------------------------------------------------------*/

#include <time.h>
#include "wise.h"

/*=====================================================================================*/
/*--  LOCAL PARAMETERS                                                                 */
/*=====================================================================================*/
static wiStatus  Status  = 0; // status code used by PSTATUS macro for line parsing
static int       Parameter;

/*=====================================================================================*/
/*--  FUNCTION PROTOTYPES                                                              */
/*=====================================================================================*/
static wiStatus wiExternal_ParseLine(wiString text);
static wiStatus wiExternal_Init(void);
static wiStatus wiExternal_Close(void);

/*=====================================================================================*/
/*--  INTERNAL ERROR HANDLING                                                          */
/*=====================================================================================*/
#define STATUS(x)    WICHK(x)
#define XSTATUS(x)   if (WICHK(x) < 0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((Status=WICHK(arg)) <= 0) return Status;

// ======================================================================================
// FUNCTION  : wiExternal_Main()
// --------------------------------------------------------------------------------------
// Purpose   : Can replace/augment wiMain() if desired
// Parameters: none
// ======================================================================================
wiStatus wiExternal_Main(void)
{
    // -------------------------------------------------
    // Open the log file and include a startup timestamp
    // -------------------------------------------------
    {
        time_t t = time(0);
        char msg[196];
        sprintf(msg," -----------------------------------------------------------------------------\n"
                    " %s Build %X (%s) starting at %s",WISE_VERSION_STRING, WISE_BUILD_VERSION, __DATE__, ctime(&t) );
        wiUtil_WriteLogFile_Direct(msg);
    }
    // --------------------------------------------------
    // Initialize the WiSE modules (including wiExternal)
    // --------------------------------------------------
    XSTATUS(wiExternal_Init());

    // -------------------------------
    // Execute through the script file
    // -------------------------------
    STATUS(wiParse_ScriptFile("wise.txt"));

    // -------------------------------
    // Shutdown the individual modules
    // -------------------------------
    STATUS(wiExternal_Close());

    // --------------------------------------------------
    // Close the log file and include an ending timestamp
    // --------------------------------------------------
    {
        time_t t = time(0);
        char msg[196];
        sprintf(msg," %s Build %X (%s) ending at %s"
                        " -----------------------------------------------------------------------------",
                        WISE_VERSION_STRING, WISE_BUILD_VERSION, __DATE__, ctime(&t));
        wiUtil_WriteLogFile_Direct(msg);
        wiUtil_CloseLogFile();
    }
    return WI_SUCCESS;
}
// end of wiExternal_Main()

// ======================================================================================
// FUNCTION  : wiExternal_Init()
// --------------------------------------------------------------------------------------
// Purpose   : Initialize modules in wiExternal.c
// Parameters: none
// ======================================================================================
wiStatus wiExternal_Init(void)
{
    XSTATUS(wiMain_Init());
    XSTATUS(wiParse_AddMethod(wiExternal_ParseLine));
    return WI_SUCCESS;
}
// end of wiExternal_Init()

// ======================================================================================
// FUNCTION  : wiExternal_Close()
// --------------------------------------------------------------------------------------
// Purpose   : Close and clean-up wiExternal.c
// Parameters: none
// ======================================================================================
wiStatus wiExternal_Close(void)
{
    XSTATUS(wiParse_RemoveMethod(wiExternal_ParseLine));
    XSTATUS(wiMain_Close());
    return WI_SUCCESS;
}
// end of wiExternal_Close()

// ======================================================================================
// FUNCTION  : wiExternal_ExampleFunction()
// --------------------------------------------------------------------------------------
// Purpose   : Sample function called from parser
// Parameters: none
// ======================================================================================
wiStatus wiExternal_ExampleFunction(void)
{
    wiPrintf(" wiExternal: Parameter = %d\n",Parameter);           // print to the console
    wiUtil_WriteLogFile(" wiExternal:Parameter = %d\n",Parameter); // create an entry in the log file; use like printf
    return WI_SUCCESS;                                             // return successfully; see wiError.h for error codes
}
// end of wiExternal_ExampleFunction()

// ======================================================================================
// FUNCTION  : wiExternal_ParseLine()
// --------------------------------------------------------------------------------------
// Purpose   : Parse a line of text for information relavent to this module
// Parameters: text -- A line of text to examine
// ======================================================================================
static wiStatus wiExternal_ParseLine(wiString text)
{
    wiStatus status;

    // -------------------
    // Look for parameters
    // -------------------
    PSTATUS( wiParse_Int (text,"wiExternal.Parameter", &Parameter, 0, 50000) );

    // -----------------------
    // Look for function calls
    // -----------------------
    XSTATUS(status = wiParse_Command(text,"wiExternal_ExampleFunction()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(wiExternal_ExampleFunction());
        return WI_SUCCESS;
    }
    return WI_WARN_PARSE_FAILED;
}
// end of wiExternal_ParseLine()

/***************************************************************************************/
/*=== END OF SOURCE CODE ==============================================================*/
/***************************************************************************************/

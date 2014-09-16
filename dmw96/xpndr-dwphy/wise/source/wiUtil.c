//--< wiUtil.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Utility Functions
//  Copyright 2001, 2005-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include "wiUtil.h"
#include "wiError.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean   Verbose       = WI_FALSE;
static wiBoolean   UseLogFile    = WI_TRUE;     // enable file logging?
static wiBoolean   SigBreakFlag  = WI_FALSE;    // a SIGBREAK signal has been received
static wiStatus    LastStatus    = WI_SUCCESS;  // last status code
static FILE       *LOGF          = NULL;        // log file handle, initially null
static wiMutex_t   LogFileMutex;

static wiBoolean   ModuleIsInitialized = WI_FALSE;
static wiBoolean   ValidLogFileMutex   = WI_FALSE;
static wiPrintfFn  wiPrintfPtr         = printf;  // ptr -> function for wiPrintf()

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(Status) WICHK((Status))
#define XSTATUS(Status) if (STATUS(Status) < 0) return WI_ERROR_RETURNED;

//=================================================================================================
//--  PRIVATE FUNCTION PROTOTYPES
//=================================================================================================
wiStatus wiUtil_InitSignalHandlers(void);
void     wiUtil_SignalHandler(int signal);
void     wiUtil_ExitHandler(void);

// ================================================================================================
// FUNCTION  : wiUtil_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiUtil_Init(void)
{
    if (ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_ALREADY_INIT);

//    STATUS( wiUtil_InitSignalHandlers() );

    ModuleIsInitialized = WI_TRUE;

    XSTATUS( wiProcess_Init() )
    XSTATUS( wiProcess_CreateMutex(&LogFileMutex) );

    ValidLogFileMutex = WI_TRUE;

	XSTATUS( wiMemory_Init() )
    XSTATUS( wiArray_Init() )

    return WI_SUCCESS;
}
// end of wiUtil_Init()

// ================================================================================================
// FUNCTION  : wiUtil_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiUtil_Close(void)
{
    if (!ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);

    XSTATUS( wiArray_Close() )
	XSTATUS( wiMemory_Close() )
    XSTATUS( wiProcess_CloseMutex(&LogFileMutex) );
    ValidLogFileMutex = WI_FALSE;

    XSTATUS( wiProcess_Close() )

    ModuleIsInitialized = WI_FALSE;
    return WI_SUCCESS;
}
// end of wiUtil_Close()

// ================================================================================================
// FUNCTION  : wiUWORD_SetUnsignedField()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return an unsigned field from a register map
// Parameters: pX  -- pointer to a UWORD
//             msb -- most significant bit
//             lsb -- least significant bit
//             val -- value to be set
// ================================================================================================
wiStatus wiUWORD_SetUnsignedField(wiUWORD *pX, int msb, int lsb, unsigned val)
{
    if ((msb<lsb) || (lsb<0) || (lsb>31) || (msb<0) || (msb>31)) {
        return WI_ERROR;
    }
    else {
        unsigned mask = ((1<<(msb-lsb+1)) - 1);
        pX->word = (pX->word & ~(mask<<lsb)) | ((val&mask)<<lsb);
        return WI_SUCCESS;
    }
}
// end of wiUWORD_SetUnsignedField()

// ================================================================================================
// FUNCTION  : wiUWORD_SetSignedField()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return an unsigned field from a register map
// Parameters: pX  -- pointer to a UWORD
//             msb -- most significant bit
//             lsb -- least significant bit
//             val -- value to be set
// ================================================================================================
wiStatus wiUWORD_SetSignedField(wiUWORD *pX, int msb, int lsb, int val)
{
    if ((msb<lsb) || (lsb<0) || (lsb>31) || (msb<0) || (msb>31)) {
        return WI_ERROR;
    }
    else {
        unsigned mask = ((1<<(msb-lsb+1)) - 1);
        pX->word = (pX->word & ~(mask<<lsb)) | ((val&mask)<<lsb);
        return WI_SUCCESS;
    }
}
// end of wiUWORD_SetSignedField()

// ================================================================================================
// FUNCTION  : wiUWORD_GetUnsignedField()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return an unsigned field from a wiUWORD
// Parameters: pX  -- pointer to a UWORD
//             msb -- most significant bit
//             lsb -- least significant bit
// ================================================================================================
unsigned int wiUWORD_GetUnsignedField(wiUWORD *pX, int msb, int lsb)
{
    if ((msb<lsb) || (lsb<0) || (lsb>31) || (msb<0) || (msb>31)) {
        STATUS(WI_ERROR);
        return 0;
    }
    else {
        unsigned mask = (1 << (msb-lsb+1)) - 1;
        return ((pX->word >> lsb) & mask);
    }
}
// end of wiUWORD_GetUnsignedField()

// ================================================================================================
// FUNCTION  : wiUWORD_GetSignedField()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return a signed field from a wiUWORD
// Parameters: pX   -- pointer to a UWORD
//             Addr -- register address
//             msb  -- most significant bit
//             lsb  -- least significant bit
// ================================================================================================
int wiUWORD_GetSignedField(wiUWORD *pX, int msb, int lsb)
{
    if ((msb<lsb) || (lsb<0) || (lsb>31) || (msb<0) || (msb>31)) {
        STATUS(WI_ERROR);
        return 0;
    }
    else {
        unsigned mask = (1 << (msb-lsb)) - 1;
        unsigned data = (pX->word >> lsb) & mask;
        unsigned sign = (pX->word >> msb) & 1;
        return  (sign? (-(signed)(mask+1)+data) : data);
    }
}
// end of wiUWORD_GetSignedField()

// ================================================================================================
// FUNCTION  : wiUtil_SetSignedWord()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus  wiUtil_SetSignedWord(int value, int high_bit, int low_bit, void *x)
{
    wiUWORD *X = (wiUWORD *)x;
    unsigned int mask, m;

    m = high_bit - low_bit;

    if (InvalidRange(value, -(1<<m), (1<<m)-1)) return WI_ERROR_PARAMETER1;
    if (InvalidRange(high_bit, 0, 31)) return WI_ERROR_PARAMETER2;
    if (InvalidRange(low_bit,  0, 31)) return WI_ERROR_PARAMETER3;
    if (high_bit < low_bit)            return WI_ERROR;
    if (!x)                            return WI_ERROR_PARAMETER4;

    mask = ((2<<m)-1) << low_bit;
    X->word = (X->word & ~mask) | (((unsigned)value<<low_bit) & mask);

    return WI_SUCCESS;
}
// end of wiUtil_SetSignedWord()

// ================================================================================================
// FUNCTION  : wiUtil_SetUnsignedWord()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus  wiUtil_SetUnsignedWord(int value, int high_bit, int low_bit, void *x)
{
    wiUWORD *X = (wiUWORD *)x;
    unsigned int mask, m;

    m = high_bit - low_bit;

    if (InvalidRange(high_bit, 0, 31)) return WI_ERROR_PARAMETER2;
    if (InvalidRange(low_bit,  0, 31)) return WI_ERROR_PARAMETER3;
    if (high_bit < low_bit)            return WI_ERROR;
    if (!x)                            return WI_ERROR_PARAMETER4;

    mask = ((2<<m)-1) << low_bit;
    X->word = (X->word & ~mask) | ((value<<low_bit) & mask);

    return WI_SUCCESS;
}
// end of wiUtil_SetUnsignedWord()

// ================================================================================================
// FUNCTION  : wiUtil_ReceivedSigBreak
// ------------------------------------------------------------------------------------------------
// Purpose   : Report whether a SIGINT or SIGBREAK event is queued
// ================================================================================================
wiBoolean wiUtil_ReceivedSigBreak(void)
{
    if (SigBreakFlag) {
        SigBreakFlag = WI_FALSE;
        return WI_TRUE;
    }
    return WI_FALSE;
}
// end of wiUtil_ReceivedSigBreak()

// ================================================================================================
// FUNCTION  : wiUtil_ErrorHandler()
// ------------------------------------------------------------------------------------------------
// Purpose   : Internal routine to set the status and report errors.
// Parameters: Status - Error codes enumerated in the header file
//             File   - Name of the file where the status was generated
//             Line   - Source code line from which the status was generated
// ================================================================================================
wiStatus wiUtil_ErrorHandler(wiStatus Status, wiString File, int Line)
{
    LastStatus = Status;

    if (Status < 0)
    {
        wiUtil_WriteLogFile_FileLine(File, Line, "0x%08X: %s\n", Status, wiUtil_ErrorMessage(Status));

        if (Verbose) 
        {
            wiPrintf("\n");
            wiPrintf(" *** WISE ERROR: %08X: %s\n",Status, wiUtil_ErrorMessage(Status));
            wiPrintf(" *** File: %s Line: %d\n",File, Line);
            wiPrintf("\n");
        }
    }
    return Status;
}
// end of wiUtil_ErrorHandler()

// ================================================================================================
// FUNCTION  : wiUtil_LastStatus
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiUtil_LastStatus(void)
{
    return LastStatus;
}
// end of wiUtil_LastStatus()

// ================================================================================================
// FUNCTION  : wiUtil_SetVerbose()
// ------------------------------------------------------------------------------------------------
// Purpose   : Set the debugging mode for the module. In verbose mode, a diagnostic message is
//             printed to the standard I/O console when a non-zero error code is returned.
// Parameters: Verbose - {0 = Silent operation; otherwise, Verbose operation}
// ================================================================================================
wiStatus wiUtil_SetVerbose(wiBoolean verbose)
{
    Verbose = verbose? WI_TRUE : WI_FALSE;
    return WI_SUCCESS;
}
// end of wiUtil_SetVerbose()

// ================================================================================================
// FUNCTION  : wiUtil_Verbose()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return the current verbose mode setting
// Parameters: none
// ================================================================================================
wiBoolean wiUtil_Verbose(void)
{
    return Verbose;
}
// end of wiUtil_Verbose()

// ================================================================================================
// FUNCTION  : wiUtil_Debug()
// ------------------------------------------------------------------------------------------------
// Purpose   : Print the current filename and line number to standard output. This function is 
//             intended to be used only for the DEBUG macro. It exists to that <stdio.h> need not be 
//             included whenever a DEBUG macro is placed into a file.
// Parameters: filename -- Filename where the DEBUG macro is placed
//             line     -- The corresponding source code line number
// ================================================================================================
wiStatus wiUtil_Debug(wiString filename, int line)
{
    wiPrintf("### FILE: %s, LINE: %d\n",filename, line);
    return WI_SUCCESS;
}
// end of wiUtil_Debug()

// ================================================================================================
// FUNCTION  : wiUtil_OpenLogFile()
// ------------------------------------------------------------------------------------------------
// Purpose   : Opens the logfile. If no name is supplied (NULL argument), the default name,
//             WISELOG.txt, is used
// ================================================================================================
wiStatus wiUtil_OpenLogFile(wiString filename)
{
    static wiBoolean append = WI_FALSE;

    //  Check if the log file is already open (return an error). Return a warning
    //  if logging is disabled.
    //
    if (LOGF) return WI_ERROR_FILE_OPEN;
    if (!UseLogFile) return WI_WARN_LOG_FILE_DISABLED;

    //  Open the logfile. If no filename is provided, the default WISELOG.txt is
    //  used. Flag append is set to true so closing and re-opening the file in
    //  the same process will cause subsequent messages to be appended rather
    //  than overwriting the file.
    //
    LOGF = fopen(filename? filename:"WISELOG.txt", append? "at":"wt");
    if (!LOGF)
    {
        wiPrintf("\n *** ERROR: Unable to open WiSE LogFile ***\n\n");
        return WI_ERROR_FILE_OPEN;
    }
    append = WI_TRUE;

    return WI_SUCCESS;
}
// end of wiUtil_OpenLogFile()

// ================================================================================================
// FUNCTION  : wiUtil_WaitForLogFileMutex()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static void wiUtil_WaitForLogFileMutex(void)
{
#if defined WISE_BUILD_MULTITHREADED

    if (ValidLogFileMutex)
    {
        if (wiProcess_WaitForMutex(&LogFileMutex) != WI_SUCCESS)
        {
            fprintf(stderr,"Unable to lock LogFileMutex. Terminating...\n");
            exit(1);
        }
    }
#endif
}

// ================================================================================================
// FUNCTION  : wiUtil_ReleaseLogFileMutex()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static void wiUtil_ReleaseLogFileMutex(void)
{
#if defined WISE_BUILD_MULTITHREADED

    if (ValidLogFileMutex)
    {
        if (wiProcess_ReleaseMutex(&LogFileMutex) != WI_SUCCESS)
        {
            fprintf(stderr,"Unable to release LogFileMutex. Terminating...\n");
            exit(1);
        }
    }
#endif
}

// ================================================================================================
// FUNCTION  : wiUtil_WriteLogFile_FileLine()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write to the WiSE file and include the location of the error
// Parameters: msg  -- Text message to log
//             file -- Filename to associate with the message
//             line -- Line number to associate with the message
// ================================================================================================
wiStatus wiUtil_WriteLogFile_FileLine(wiString file, int line, const char *format, ...)
{
    if (UseLogFile)
    {
        va_list ap;
        wiUtil_WaitForLogFileMutex();

        if (!LOGF) {
            if (wiUtil_OpenLogFile(0))
                return WI_ERROR_FILE_OPEN;
        }

        while (strchr(file,'\\'))        // Strip off the path portion of the filename
            file = strchr(file,'\\') + 1; 

        if (wiProcess_ThreadIndex() != 0)
            fprintf(LOGF," [(%d)%s:%d] ", wiProcess_ThreadIndex(), file, line);
        else
            fprintf(LOGF," [%s:%d] ", file, line);
        va_start(ap,format); vfprintf(LOGF,format,ap); va_end(ap);
        fflush(LOGF);

        wiUtil_ReleaseLogFileMutex();

        if (Verbose) {
            va_start(ap,format); vprintf(format,ap); va_end(ap);
        }
    }
    return WI_SUCCESS;
}
// end of wiUtil_WriteLogFile_FileLine()

// ================================================================================================
// FUNCTION  : wiUtil_WriteLogFile()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write to the WiSE log file
// Parameters: same as printf()
// ================================================================================================
int wiUtil_WriteLogFile(const char *format,...)
{
    int n = 0;

    if (UseLogFile)
    {
        va_list ap;
        wiUtil_WaitForLogFileMutex();

        if (!LOGF) {
            if (wiUtil_OpenLogFile(0))
                return WI_ERROR_FILE_OPEN;
        }

        va_start(ap,format); n = vfprintf(LOGF,format,ap); va_end(ap);
        fflush(LOGF);

        wiUtil_ReleaseLogFileMutex();

        if (Verbose) 
        {
            if (n < 1020)
            {
                char buf[1024];
                va_start(ap,format);   
                vsprintf(buf,format,ap); 
                va_end(ap);
                wiPrintf("%s",buf);
            }
            else
            {
                va_start(ap,format);   
                vprintf(format,ap); 
                va_end(ap);
            }
        }
        return WI_SUCCESS;
    }
    return n;
}
// end of wiUtil_WriteLogFile()

// ================================================================================================
// FUNCTION  : wiUtil_CloseLogFile()
// ------------------------------------------------------------------------------------------------
// Purpose   : Close the current log file. This function should be called when the application
//             is shut down.
// Parameters: none
// ================================================================================================
wiStatus wiUtil_CloseLogFile(void)
{
    if (LOGF) fclose(LOGF);
    LOGF = NULL;

    return WI_SUCCESS;
}
// end of wiUtil_CloseLogFile()

// ================================================================================================
// FUNCTION  : wiUtil_UseLogFile()
// ------------------------------------------------------------------------------------------------
// Purpose   : Specifies whether messages should be recorded in the WiSE log file
// Parameters: state = { WI_TRUE, WI_FALSE }
// ================================================================================================
wiStatus wiUtil_UseLogFile(wiBoolean state)
{
    UseLogFile = state;
    return WI_SUCCESS;
}
// end of wiUtil_UseLogFile()

// ================================================================================================
// FUNCTION  : wiUtil_ErrorMessage()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return a text description of a status/error code
// Parameters: status  -- WiSE status code
//             message -- character buffer for the text description (length of at least 128 bytes)
// ================================================================================================
const char * wiUtil_ErrorMessage(wiStatus status)
{
    static const char UnknownStatusCode[] = {"Unknown Status Code"};
    static const struct {
        wiStatus status; wiString description;
    } statusDescArray[] = 
    {
        // Normal Status--------------------------------------------------------
        {WI_SUCCESS,                        "WISE function succeeded."},

        // Warnings-------------------------------------------------------------
        {WI_WARN_UNKNOWN_STATUS,            "WARNING: The WISE status code is not defined."},
        {WI_WARN_END_OF_FILE,               "WARNING: End-of-file has been reached."},
        {WI_WARN_BUFFER_OVERFLOW,           "WARNING: String value truncated due to buffer overflow."},
        {WI_WARN_FUNCTION_UNAVAILABLE,      "WARNING: Called function not executed"},
        {WI_WARN_LOG_FILE_DISABLED,         "WARNING: Writing to the log file is disabled"},
        {WI_WARN_WINDOWS_ONLY,              "WARNING: Function available only on Windows platforms"},
        {WI_WARN_WINDOWS_NT_ONLY,           "WARNING: Function available only on WindowsNT platforms"},
        {WI_WARN_PARSE_FAILED,              "WARNING: Parsing function unable to handle the text line"},
        {WI_WARN_METHOD_UNHANDLED_PARAMETER,"WARNING; The parameter was not handled by the method function"},
        {WI_WARN_METHOD_UNHANDLED_COMMAND,  "WARNING: Command/parameter was not handled by the method function"},

        // Modem Operation -----------------------------------------------------
        {WI_WARN_PHY_SIGNAL_PARITY,         "WARNING: Parity error in the SIGNAL field"},
        {WI_WARN_PHY_SIGNAL_RATE,           "WARNING: Illegal value in the decoded RATE field"},
        {WI_WARN_PHY_SIGNAL_LENGTH,         "WARNING: Decoded LENGTH value in SIGNAL field is incorrect"},
        {WI_WARN_PHY_BAD_PACKET,            "WARNING: Packet reception failed in PHY"},

        // Errors---------------------------------------------------------------
        {WI_ERROR,                          "ERROR: Function returned an error"},
        {WI_ERROR_FILE_OPERATION,           "ERROR: Unable to complete the file operation"},
        {WI_ERROR_MEMORY_ALLOCATION,        "ERROR: Memory allocation failed"},
        {WI_ERROR_FILE_OPEN,                "ERROR: Opening the specified file"},
        {WI_ERROR_FILE_WRITE,               "ERROR: Writing to the specified file"},
        {WI_ERROR_FILE_READ,                "ERROR: Reading to the specified file"},
        {WI_ERROR_FILE_FORMAT,              "ERROR: Incorrect file format"},
        {WI_ERROR_FUNCTION_UNAVAILABLE,     "ERROR: Called function is not availabled"},
        {WI_ERROR_UNDEFINED_CASE,           "ERROR: Undefine case in switch-case structure"},
        {WI_ERROR_RETURNED,                 "ERROR: A called function returned an error"},
        {WI_ERROR_DIVIDE_BY_ZERO,           "ERROR: Function about to divide by zero"},
        {WI_ERROR_PARSE_FAILED,             "ERROR: Parsing of the script failed"},
        {WI_ERROR_PARSE_PARAMETER_RANGE,    "ERROR: Parsing failed due to a parameter out of range"},
        {WI_ERROR_MODULE_NOT_INITIALIZED,   "ERROR: The module containing the called function has not been initialized"},
        {WI_ERROR_MODULE_ALREADY_INIT,      "ERROR: The module is already initialized"},
        {WI_ERROR_INITIALIZING_MODULE,      "ERROR: Unable to initialize the module"},
        {WI_ERROR_INVALID_METHOD,           "ERROR: Active method is invalid (usually not selected)."},
        {WI_ERROR_METHOD_COMMAND,           "ERROR: Illegal method command."},
        {WI_ERROR_METHOD_PARAM,             "ERROR: Illegal method parameter."},
        {WI_ERROR_METHOD_PARAM_RANGE,       "ERROR: Method parameter value is out of range."},
        {WI_ERROR_METHOD_UNHANDLED_COMMAND, "ERROR: Command/parameter was not handled by the method function."},
        {WI_ERROR_RANGE,                    "ERROR: Parameter/Value is out of range"},
        {WI_ERROR_PARAMETER1,               "ERROR: Parameter 1 is out of range"},
        {WI_ERROR_PARAMETER2,               "ERROR: Parameter 2 is out of range"},
        {WI_ERROR_PARAMETER3,               "ERROR: Parameter 3 is out of range"},
        {WI_ERROR_PARAMETER4,               "ERROR: Parameter 4 is out of range"},
        {WI_ERROR_PARAMETER5,               "ERROR: Parameter 5 is out of range"},
        {WI_ERROR_PARAMETER6,               "ERROR: Parameter 6 is out of range"},
        {WI_ERROR_PARAMETER7,               "ERROR: Parameter 7 is out of range"},
        {WI_ERROR_PARAMETER8,               "ERROR: Parameter 8 is out of range"},
        {WI_ERROR_PARAMETER9,               "ERROR: Parameter 9 is out of range"},
        {WI_ERROR_PARAMETER10,              "ERROR: Parameter 10 is out of range"},
        {WI_ERROR_PARAMETER11,              "ERROR: Parameter 11 is out of range"},
        {WI_ERROR_PARAMETER12,              "ERROR: Parameter 12 is out of range"},
        {WI_ERROR_PARAMETER13,              "ERROR: Parameter 13 is out of range"},
        {WI_ERROR_PARAMETER14,              "ERROR: Parameter 14 is out of range"},
        {WI_ERROR_PARAMETER15,              "ERROR: Parameter 15 is out of range"},
        {WI_ERROR_PARAMETER16,              "ERROR: Parameter 16 is out of range"},
        {WI_ERROR_PARAMETER17,              "ERROR: Parameter 17 is out of range"},
        {WI_ERROR_PARAMETER18,              "ERROR: Parameter 18 is out of range"},
        {WI_ERROR_PARAMETER19,              "ERROR: Parameter 19 is out of range"},
        {WI_ERROR_PARAMETER20,              "ERROR: Parameter 20 is out of range"},
        {WI_ERROR_MATRIX_SIZE_MISMATCH,     "ERROR: Matrix size mismatch"},
        {WI_ERROR_MATRIX_INDEX_OUT_OF_RANGE,"ERROR: Matrix index out of range"},
        {WI_ERROR_WINDOWS_ONLY,             "ERROR: Feature is only available in Win32 builds"},
        {WI_ERROR_WINDOWS_NT_ONLY,          "ERROR: Feature is only available on WindowsNT platforms"},
        {WI_ERROR_INVALID_FOR_THREAD,       "ERROR: Function not valid when called from a thread"},
        {WI_ERROR_NO_MULTITHREAD_SUPPORT,   "ERROR: Multithreaded operation is not supported"},
        // List terminator------------------------------------------------------
        {WI_SUCCESS, NULL}
    };
    int i;

    for (i=0; statusDescArray[i].description; i++) {
        if (statusDescArray[i].status == status) {
            return statusDescArray[i].description;
        }
    }
    return UnknownStatusCode;
}
// end of wiUtil_ErrorMessage()

// ================================================================================================
// FUNCTION  : dump_wiByte()
// ------------------------------------------------------------------------------------------------
// Purpose   : Dump a data array to a text file
// Parameters: filename -- File to which the data are saved
//             x        -- Pointer to the data array
//             n        -- Number of elements to write
// ================================================================================================
wiStatus dump_wiByte(wiString filename, wiByte *x, int n)
{
    FILE *F; int i;

    if (Verbose) wiPrintf(" dump_wiByte(\"%s\",0x%08X,%d)\n",filename,x,n);
    if (InvalidRange(n,  1, 1048576)) return WI_ERROR_PARAMETER3;

    F = fopen((char *)filename, "w");
    if (!F) return WI_ERROR_FILE_OPEN;

    for (i=0; i<n; i++, x++)
        fprintf(F,"0x%02X\n",*x);
    fclose(F);
    return WI_SUCCESS;
}
// end of dump_wiByte()

// ================================================================================================
// FUNCTION  : dump_wiInt()
// ------------------------------------------------------------------------------------------------
// Purpose   : Dump a data array to a text file
// Parameters: filename -- File to which the data are saved
//             x        -- Pointer to the data array
//             n        -- Number of elements to write
// ================================================================================================
wiStatus dump_wiInt(wiString filename, wiInt *x, int n)
{
    FILE *F; int i;

    if (Verbose) wiPrintf(" dump_wiInt(\"%s\",0x%08X,%d)\n",filename,x,n);
    if (InvalidRange(n,  1, 1048576)) return WI_ERROR_PARAMETER3;

    F = fopen((char *)filename, "w");
    if (!F) return WI_ERROR_FILE_OPEN;

    for (i=0; i<n; i++, x++)
        fprintf(F,"% 3d\n",*x);
    fclose(F);
    return WI_SUCCESS;
}
// end of dump_wiInt()

// ================================================================================================
// FUNCTION  : dump_wiReal()
// ------------------------------------------------------------------------------------------------
// Purpose   : Dump a data array to a text file
// Parameters: filename -- File to which the data are saved
//             x        -- Pointer to the data array
//             n        -- Number of elements to write
// ================================================================================================
wiStatus dump_wiReal(wiString filename, wiReal *x, int n)
{
    FILE *F; int i;

    if (Verbose) wiPrintf(" dump_wiReal(\"%s\",0x%08X,%d)\n",filename,x,n);
    if (InvalidRange(n, 1, 1048576)) return WI_ERROR_PARAMETER3;

    F = fopen(filename, "wt");
    if (!F) return WI_ERROR_FILE_OPEN;

    for (i=0; i<n; i++, x++)
        fprintf(F,"% 10.4e\n",*x);
    fclose(F);

    return WI_SUCCESS;
}
// end of dump_wiReal()

// ================================================================================================
// FUNCTION  : dump_wiComplex()
// ------------------------------------------------------------------------------------------------
// Purpose   : Dump a data array to a text file
// Parameters: filename -- File to which the data are saved
//             x        -- Pointer to the data array
//             n        -- Number of elements to write
// ================================================================================================
wiStatus dump_wiComplex(wiString filename, wiComplex *x, int n)
{
    FILE *F; int i;

    if (Verbose) wiPrintf(" dump_wiComplex(\"%s\",0x%08X,%d)\n",filename,x,n);

    F = fopen(filename, "wt");
    if (!F) return WI_ERROR_FILE_OPEN;

    for (i=0; i<n; i++) {
        fprintf(F, "% 10.4e % 10.4e\n",( float)(x[i].Real), (float)(x[i].Imag));
    }
    fclose(F);
    return WI_SUCCESS;
}
// end of dump_wiComplex()

// ================================================================================================
// FUNCTION  : dump_wiWORD()
// ------------------------------------------------------------------------------------------------
// Purpose   : Dump a data array to a text file
// Parameters: filename -- File to which the data are saved
//             x        -- Pointer to the data array
//             n        -- Number of elements to write
// ================================================================================================
wiStatus dump_wiWORD(wiString filename, wiWORD *x, int n)
{
    FILE *F; int i;

    if (Verbose) wiPrintf(" dump_wiWORD(\"%s\",0x%08X,%d)\n",filename,x,n);
    if (InvalidRange(n, 1, 1048576)) return WI_ERROR_PARAMETER3;

    F = fopen((char *)filename, "w");
    if (!F) return WI_ERROR_FILE_OPEN;

    for (i=0; i<n; i++, x++)
        fprintf(F,"%6d\n",x->word);
    fclose(F);
    return WI_SUCCESS;
}
// end of dump_wiWORD()

// ================================================================================================
// FUNCTION  : dump_wiUWORD()
// ------------------------------------------------------------------------------------------------
// Purpose   : Dump a data array to a text file
// Parameters: filename -- File to which the data are saved
//             x        -- Pointer to the data array
//             n        -- Number of elements to write
// ================================================================================================
wiStatus dump_wiUWORD(wiString filename, wiUWORD *x, int n)
{
    FILE *F; int i;

    if (Verbose) wiPrintf(" dump_wiUWORD(\"%s\",0x%08X,%d)\n",filename,x,n);
    if (InvalidRange(n, 1, 1048576)) return WI_ERROR_PARAMETER3;

    F = fopen((char *)filename, "w");
    if (!F) return WI_ERROR_FILE_OPEN;

    for (i=0; i<n; i++, x++)
        fprintf(F,"%6u\n",x->word);
    fclose(F);
    return WI_SUCCESS;
}
// end of dump_wiUWORD()

// ================================================================================================
// FUNCTION  : dump_wiUWORD_pair()
// ------------------------------------------------------------------------------------------------
// Purpose   : Dump a pair of data array to a text file
// Parameters: filename -- File to which the data are saved
//             x        -- Pointer to the first data array
//             y        -- Pointer to the second data array
//             n        -- Number of elements to write
// ================================================================================================
wiStatus dump_wiWORD_pair(wiString filename, wiWORD *x, wiWORD *y, int n)
{
    FILE *F; int i;

    if (Verbose) wiPrintf(" dump_wiWORD_pair(\"%s\",0x%08X,0x%08X,%d)\n",filename,x,y,n);
    if (InvalidRange(n, 1, 1048576)) return WI_ERROR_PARAMETER3;

    F = fopen((char *)filename, "w");
    if (!F) return WI_ERROR_FILE_OPEN;

    for (i=0; i<n; i++, x++, y++)
        fprintf(F,"%6d %6d\n",x->word,y->word);
    fclose(F);
    return WI_SUCCESS;
}
// end of dump_wiWORD_pair()

// ================================================================================================
// FUNCTION  : wiUtil_Comma()
// ------------------------------------------------------------------------------------------------
// Purpose   : Create a string presentation of an integer with comma characters in 000s location
// Parameters: x -- Interger value
// Returns   : String containing the value
// ================================================================================================
wiString wiUtil_Comma(unsigned int x)
{
    static char outString[WISE_MAX_THREADS][32];
    char s1[32] = "";
    char s2[32];

    int ThreadIndex = wiProcess_ThreadIndex();

    if (!x) strcpy(s1,"0\0");

    while (x)
    {
        if (x < 1000) sprintf(s2,   "%i",(x % 1000));
        else          sprintf(s2,",%03i",(x % 1000));
        strcat(s2,s1);
        strcpy(s1,s2);
        x = x / 1000;
    }
    strcpy(outString[ThreadIndex], s1);
    return outString[ThreadIndex];
}
// end of wiUtil_Comma()

// ================================================================================================
// FUNCTION  : wiUtil_BitStr()
// ------------------------------------------------------------------------------------------------
// Purpose   : Create a string presentation of a Boolean value
// Parameters: Val -- Boolean value
// Returns   : String containing the value
// ================================================================================================
wiString wiUtil_Boolean(wiBoolean x)
{
    static char outString[WISE_MAX_THREADS][32];
    int ThreadIndex = wiProcess_ThreadIndex();

    sprintf(outString[ThreadIndex], (x? "TRUE":"FALSE"));
    return outString[ThreadIndex];
}
// end of wiUtil_Boolean()

// ================================================================================================
// FUNCTION  : wiUtil_BitStr()
// ------------------------------------------------------------------------------------------------
// Purpose   : Create a string presentation of a number in binary form
// Parameters: x -- Interger value
//             n -- Number of significant bits
// Returns   : String containing the value
// ================================================================================================
wiString wiUtil_BinStr(int x, int n)
{
    static char outString[WISE_MAX_THREADS][48];
    int ThreadIndex = wiProcess_ThreadIndex();
    int i;

    if (n > 32) n = 32; // prevent errors

    for (i=0; i<n; i++)
        outString[ThreadIndex][n-i-1] = (char)(((x>>i) & 1) ? '1':'0');
    outString[ThreadIndex][n] = '\0';

    return outString[ThreadIndex];
}
// end of wiUtil_BinStr()

// ================================================================================================
// FUNCTION  : wiUtil_PrintfPtr()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a pointer to the registered "printf" function
// ================================================================================================
wiPrintfFn wiUtil_PrintfPtr(void)
{
    return wiPrintfPtr;
}
// end of wiUtil_PrintfPtr()

// ================================================================================================
// FUNCTION  : wiUtil_SetPrintf()
// ------------------------------------------------------------------------------------------------
// Purpose   : Provide a pointer to a function to be used for wiPrintf calls
// ================================================================================================
wiStatus wiUtil_SetPrintf(wiPrintfFn printfFn)
{
    if (!printfFn) return WI_ERROR_PARAMETER1;

    wiPrintfPtr = printfFn;
    wiUtil_WriteLogFile(" wiUtil_SetPrintf(%p)\n", printfFn);

    return WI_SUCCESS;
}
// end of wiUtil_SetPrintf()

// ================================================================================================
// FUNCTION  : wiUtil_InitSignalHandlers()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiUtil_InitSignalHandlers(void)
{
    signal(SIGABRT,  wiUtil_SignalHandler);
    signal(SIGINT,   wiUtil_SignalHandler);
    signal(SIGTERM,  wiUtil_SignalHandler);
    signal(SIGSEGV,  wiUtil_SignalHandler);
#ifdef SIGBREAK
//    signal(SIGBREAK, wiUtil_SignalHandler);
#endif

    atexit(wiUtil_ExitHandler);

    return WI_SUCCESS;
}
// end of wiUtil_InitSignalHandlers()

// ================================================================================================
// FUNCTION  : wiUtil_SignalHandler()
// ------------------------------------------------------------------------------------------------
// Purpose   : Handle 'C' signals
// Parameters: sig -- the signal to be handled
// ================================================================================================
void wiUtil_SignalHandler(int sig)
{
    switch (sig)
    {
        case SIGABRT: wiUtil_WriteLogFile("Terminating on SIGABRT\n");   raise(SIGABRT); break;
        case SIGSEGV: wiUtil_WriteLogFile("Program received SIGSEGV\n"); raise(SIGSEGV); break;
        case SIGTERM: wiUtil_WriteLogFile("Terminating on SIGTERM\n");   raise(SIGTERM); break;

        case SIGINT : wiUtil_WriteLogFile("Received SIGINT\n");
            if (!SigBreakFlag) {
                SigBreakFlag = WI_TRUE;
                signal(SIGINT, wiUtil_SignalHandler);
            }
            else raise(SIGINT);
            break;

#ifdef SIGBREAK
        case SIGBREAK: wiUtil_WriteLogFile("Program received SIGBREAK\n");
            if (!SigBreakFlag) {
                SigBreakFlag = WI_TRUE;
                signal(SIGBREAK, wiUtil_SignalHandler);
            }
            else raise(SIGBREAK);
            break;
#endif

        default: STATUS(WI_ERROR_UNDEFINED_CASE);
    }
}
// end of wiUtil_SignalHandler()

// ================================================================================================
// FUNCTION  : wiUtil_ExitHandler()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiUtil_ExitHandler(void)
{
    if (LOGF) {
        wiUtil_WriteLogFile("Program exiting...\n"); 
        wiUtil_CloseLogFile();
    }
}
// end of wiUtil_ExitHandler()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

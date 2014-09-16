//--< wiParse.c >----------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Script Parsing Utilities
//           
//  Copyright 2001-2003 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.
//      Portions also Copyright 1995-1998 Barrett Brickner. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // required for multiple types of string manipulation
#include <stdarg.h>   // required for wiParse_Function()
#include <float.h>
#include <limits.h>
#include "wiUtil.h"
#include "wiParse.h"
#include "wiRnd.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean Verbose       = 0;
static wiParse_State_t *pState = NULL;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (WICHK(arg)<0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((pstatus=WICHK(arg)) <= 0) return pstatus;

//=================================================================================================
//--  LOCAL FUNCTION PROTOTYPES
//=================================================================================================
wiBoolean wiParse_TextIsWhiteSpace(char *cptr);
wiStatus  wiParse_DoubleValue(char **pptr, double *dval);

// ================================================================================================
// FUNCTION  : wiParse_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Clean up any open files or dynamic memory at exit
// ================================================================================================
wiStatus wiParse_Init(void)
{
    if (pState) return STATUS(WI_ERROR_MODULE_ALREADY_INIT);

    WICALLOC( pState, 1, wiParse_State_t );
    pState->AbortOnError = 1;

    return WI_SUCCESS;
}
// end of wiParse_Init()

// ================================================================================================
// FUNCTION  : wiParse_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Clean up any open files or dynamic memory at exit
// ================================================================================================
wiStatus wiParse_Close(void)
{
    int i;

    if (!pState) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);

    if (pState->ScriptIsPaused)
    {
        if (pState->PausedFileName) wiFree(pState->PausedFileName); pState->PausedFileName = NULL;
        if (pState->PausedFile)     fclose(pState->PausedFile);     pState->PausedFile = NULL;
    }
    while (pState->MethodList)
        wiParse_RemoveMethod(pState->MethodList->fn);

    for (i=0; i<WIPARSE_MAX_FILEPATHS; i++) {
        if (pState->FilePath[i]) wiFree(pState->FilePath[i]);
    }
    wiFree(pState); pState = NULL;

    return WI_SUCCESS;
}
// end of wiParse_Close()

// ================================================================================================
// FUNCTION  : wiParse_AddMethod()
// ------------------------------------------------------------------------------------------------
// Purpose   : Add a parsing function to the method list.
// Parameters: Function -- External 'C' function pointer
// ================================================================================================
wiStatus wiParse_AddMethod(wiParseFunction_t fn)
{
    wiParseMethod_t *list, *method;

    if (Verbose) wiPrintf(" wiParse_AddMethod(%d,@x%08X)\n",fn);
    if (!fn) return STATUS(WI_ERROR_PARAMETER1);

    // Create a new method record
    // 
    WICALLOC( method, 1, wiParseMethod_t );

    method->fn   = fn;
    method->next = NULL;
    method->prev = NULL;

    // Add the new method to the list
    // 
    if (pState->MethodList) 
    {
        for (list = pState->MethodList; list->next; list = list->next) ;
        list->next = method;
        method->prev = list;
    }
    else pState->MethodList = method;

    return WI_SUCCESS;
}
// end of wiParse_AddMethod()

// ================================================================================================
// FUNCTION  : wiParse_RemoveMethod() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Remove a parsing function from the method list
// Parameters: Function -- External 'C' function pointer
// ================================================================================================
wiStatus wiParse_RemoveMethod(wiParseFunction_t fn)
{
    wiParseMethod_t *method;

    if (Verbose) wiPrintf(" wiParse_RemoveMethod(%d,@x%08X)\n",fn);
    if (!fn) return STATUS(WI_ERROR_PARAMETER1);

    // ---------------------
    // Search for the method
    // ---------------------
    for (method = pState->MethodList; method; method = method->next) {
        if (method->fn == fn) break;
    }
    // ---------------------------------------------------
    // If the method function was found, remove the record
    // ---------------------------------------------------
    if (method)
    {
        if (pState->MethodList == method) pState->MethodList = method->next;
        if (method->next) (method->next)->prev = method->prev;
        if (method->prev) (method->prev)->next = method->next;
        wiFree(method);
    }
    else return STATUS(WI_ERROR_PARAMETER1);

    return WI_SUCCESS;
}
// end of wiParse_RemoveMethod()

// ================================================================================================
// FUNCTION  : wiParse_ExitScript()
// ------------------------------------------------------------------------------------------------
// Purpose   : Set a flag to terminate the current script processing 
// ================================================================================================
wiStatus wiParse_ExitScript(void)
{
    wiUtil_WriteLogFile("wiParse_ExitScript\n");
    pState->ExitScript = 1;
    return WI_SUCCESS;
}
// end of wiParse_ExitScript()    

// ================================================================================================
// FUNCTION  : wiParse_PauseScript()
// ------------------------------------------------------------------------------------------------
// Purpose   : Set a flag to pause script file parsing
// ================================================================================================
wiStatus wiParse_PauseScript(void)
{
    if (pState->RecursionDepth == 1)
        pState->PauseScript = 1;
    else
    {
        if (pState->RecursionDepth == 0)
            wiUtil_WriteLogFile("Attempt to pause a script processing when no file is active\n");
        else
            wiUtil_WriteLogFile("Only the top-level script can be paused; RecursionDepth = %d\n",
            pState->RecursionDepth);

        return STATUS(WI_ERROR);
    }
    return WI_SUCCESS;
}
// end of wiParse_PauseScript()    

// ================================================================================================
// FUNCTION  : wiParse_ScriptIsPaused()
// ------------------------------------------------------------------------------------------------
// Purpose   : Inidicate whether the parser paused processing a script file
// ================================================================================================
wiBoolean wiParse_ScriptIsPaused(void)
{
    return pState->ScriptIsPaused;
}
// end of wiParse_ScriptIsPaused()    

// ================================================================================================
// FUNCTION  : wiParse_ScriptFileEngine() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Core processing engine for script files
// Parameters: filename -- Name of file to parse 
// ================================================================================================
wiStatus wiParse_ScriptFileEngine(FILE *F, char *filename, int line)
{
    wiStatus status = WI_SUCCESS;
    char *buf, *text;

    if (Verbose) wiPrintf(" wiParse_ScriptFileEngine(*)\n");

    if (!F) return STATUS(WI_ERROR_PARAMETER1);

    // -----------------
    // Memory Allocation
    // -----------------
    WICALLOC( buf,  WIPARSE_MAX_LINE_LENGTH + 2, char);
    WICALLOC( text, WIPARSE_MAX_LINE_LENGTH + 1, char);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Process the intput file until either the end-of-file or until either the 
    //  ExitScript or PauseScript flags are set.
    //
    while (!feof(F) && !pState->ExitScript && !pState->PauseScript)
    {
        // --------------------------------------------
        // Read a newline-terminated line from the file
        // --------------------------------------------
        if (fgets(buf, WIPARSE_MAX_LINE_LENGTH+1, F) == NULL ) // if fgets() fails...
        {
            if (feof(F)) continue;              // if end of file, continue to exit the loop
            status = WI_ERROR_FILE_READ; break; // otherwise, status=error and break the loop
        }
        line++;

        // ------------------------------------
        // Check the length for buffer overflow
        // ------------------------------------
        if (strlen(buf) > WIPARSE_MAX_LINE_LENGTH) 
        {
            status = STATUS(WI_ERROR);
            break;
        }
        // -----------------------
        // Clean the buffer string
        // -----------------------
        status = wiParse_CleanLine(buf, text, WIPARSE_MAX_LINE_LENGTH);
        if (status) break; // if there was an error, exit the loop

        // ------------------------
        // Skip blank/comment lines
        // ------------------------
        if (!strlen(text)) continue;

        // --------------
        // Parse the line
        // --------------
        if (!status) 
        {
            if (wiParse_Line(text) == WI_SUCCESS) continue;
        }
        // ------------------------------------------------
        // The line was not handled, so process as an error
        // ------------------------------------------------
        {
            char *cptr;

            for (cptr = buf + strlen(buf) - 1; cptr > buf; cptr--) 
            {
                if ((*cptr==' ') || (*cptr=='\t') || (*cptr=='\n'))
                    *cptr = '\0';
                else break;
            }
            wiUtil_WriteLogFile(" Error in \"%s\" at line %d: \"%s\"\n", filename, line, buf);
        }
        // -----------------------------
        // Abort on error if so directed
        // -----------------------------
        if (pState->AbortOnError) 
        {
            status = STATUS(WI_ERROR_PARSE_FAILED);
            break;
        }
    }
    //-----  end of while (!feof(F))  -----------------------------------------

    if (pState->PauseScript)
    {
        if (pState->PausedFileName) 
        {
            if (strcmp(filename, pState->PausedFileName) != 0)
                return STATUS(WI_ERROR);
        }
        else
        {
            pState->PausedFileName = (char *)wiMalloc( strlen(filename) + 1);
            if (!pState->PausedFileName) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
            strcpy(pState->PausedFileName, filename);
        }
        pState->PausedFile = F;
        pState->PausedLine = line;
        pState->ScriptIsPaused = 1;
        pState->PauseScript = 0;

        wiUtil_WriteLogFile("Parsing \"%s\" paused at line %d\n", filename, line);
    }

    // Clean up and return
    //
    wiFree(buf); wiFree(text);

    return status;
}
// end of wiParse_ScriptFileEngine()

// ================================================================================================
// FUNCTION  : wiParse_OpenFile() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Open a file using the wiParse FilePath options
// Parameters: filename -- Name of file to open
//             mode     -- Argument for fopen
// ================================================================================================
FILE * wiParse_OpenFile(wiString filename, wiString mode)
{
    FILE *F = NULL;
    int i;

    if ((F = fopen(filename, mode)) == NULL)
    {
        for (i=0; i<WIPARSE_MAX_FILEPATHS; i++)
        {
            if (!pState->FilePath[i]) continue;
            if (strlen(pState->FilePath[i]) + strlen(filename) + 2 > WIPARSE_MAX_FILENAME_LENGTH)
            {
                wiUtil_WriteLogFile("Path + File name length exceeds buffer length\n");
                return NULL;
            }
            sprintf(pState->LastFileName, "%s/%s",pState->FilePath[i],filename);
            if ((F = fopen(pState->LastFileName,"r")) != NULL) break;
        }
    }
    else strncpy(pState->LastFileName, filename, WIPARSE_MAX_FILENAME_LENGTH-1);

    return F;
}
// end of wiParse_OpenFile()

// ================================================================================================
// FUNCTION  : wiParse_ScriptFile() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Start the script parser 
// Parameters: filename -- Name of file to parse 
// ================================================================================================
wiStatus wiParse_ScriptFile(wiString filename)
{
    FILE *F; char *buf = NULL;

    if (Verbose) wiPrintf(" wiParse_ScriptFile(\"%s\")\n",filename);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Error checking. Check for the case where either the input file is not specified
    //  or the recursion depth has been exceeded.
    //
    if (pState->RecursionDepth > WIPARSE_MAX_RECURSION_DEPTH)
    {
        wiUtil_WriteLogFile("Error: Maximum recursing depth exceeded in wiParse.\n");
        return STATUS(WI_ERROR);
    }
    if (!filename) return WI_ERROR_PARAMETER1;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Resume a paused script file. If ScriptIsPaused is set and the input filename
    //  matches that of the paused script, assume the intent is to resume parsing.
    //
    if (pState->ScriptIsPaused)
    {
        if (strcmp(filename, pState->PausedFileName) == 0)
        {
            XSTATUS( wiParse_ResumeScript() );
            return WI_SUCCESS;
        }
        else
        {
            wiUtil_WriteLogFile("Attempt to parse a script file \"%s\" while another is paused\n",filename);
            return STATUS(WI_ERROR);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Parse the specified file
    //
    if ((F = wiParse_OpenFile(filename,"r")) == NULL)
    {
        wiUtil_WriteLogFile("Unable to open file \"%s\" for parsing.\n",filename);
        return WI_ERROR_FILE_OPEN;
    }
    wiUtil_WriteLogFile("[%d] wiParse_ScriptFile(\"%s\")\n",pState->RecursionDepth,pState->LastFileName);

    pState->ExitScript = 0;   // clear flag to exit script file
    pState->RecursionDepth++; // increase the recursion depth counter

    XSTATUS( wiParse_ScriptFileEngine(F, filename, 0) );
    if (buf) wiFree(buf);

    if (!pState->ScriptIsPaused) {
        pState->RecursionDepth--;
        fclose(F);
    }

    return WI_SUCCESS;
}
// end of wiParse_ScriptFile()

// ================================================================================================
// FUNCTION  : wiParse_ResumeScript() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Resume a paused script file
// Parameters: filename -- Name of file to parse 
// ================================================================================================
wiStatus wiParse_ResumeScript(void)
{
    if (pState->ScriptIsPaused)
    {
        wiUtil_WriteLogFile("wiParse_ResumeScript\n");

        pState->ScriptIsPaused = 0;
        XSTATUS( wiParse_ScriptFileEngine(pState->PausedFile, pState->PausedFileName, pState->PausedLine) );

        if (!pState->ScriptIsPaused)
        {
            fclose(pState->PausedFile);
            pState->RecursionDepth--;
            if (pState->PausedFileName) wiFree(pState->PausedFileName); 
            pState->PausedFileName = NULL;
        }
        return WI_SUCCESS;
    }
    else
    {
        wiUtil_WriteLogFile("Attempt to resume script file when none is paused.\n");
        return WI_WARNING;
    }
}
// end of wiParse_ResumeScript()

// ================================================================================================
// FUNCTION  : wiParse_CleanLine() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Remove comments and whitespace from a text line.
// Parameters: InString  -- Text line to be cleaned 
//             OutString -- cleaned version of the input string
//             MaxLength  -- Maximum line length (1..4096) 
// ================================================================================================
wiStatus wiParse_CleanLine(wiString InString, char *OutString, wiUInt MaxLength)
{
    char *cptr = InString;

    if (!InString)  return WI_ERROR_PARAMETER1;
    if (!OutString) return WI_ERROR_PARAMETER2;
    if (InvalidRange(MaxLength, 1, 4096)) return WI_ERROR_PARAMETER4;
    if (strlen(InString) > MaxLength) return WI_ERROR_PARAMETER1;

    
    if ( (InString[0]=='#') || (InString==strstr(InString,"//")) ) { // Check for comment only line
        *OutString = '\0';
        return WI_SUCCESS;
    }

    while ((*cptr==' ') || (*cptr=='\t'))  // Find the first non-blank, non-tab character
        cptr++;                           

    strcpy(OutString, cptr);  // Copy the input to the output buffer

    if ((cptr = strchr(OutString, '#')) != NULL) *cptr = '\0'; // Remove any trailing comments
    if ((cptr = strstr(OutString,"//")) != NULL) *cptr = '\0'; //    "    "      "       "

    if ((cptr = strchr(OutString,'\n')) != NULL) *cptr = '\0'; // Terminate at the newline character
    if ((cptr = strchr(OutString,'\r')) != NULL) *cptr = '\0'; // Terminate at a carriage return

    if (strlen(OutString) == 0) return WI_SUCCESS;             // Exit if a zero-length string

    // Remove any trailing blanks and tabs starting from the end
    //
    for (cptr = OutString + strlen(OutString) - 1; cptr > OutString; cptr--)
    {
        if ((*cptr==' ') || (*cptr=='\t'))
            *cptr = '\0';                 
        else break;                        
    }
    return WI_SUCCESS;
}
// end of wiParse_CleanLine()

// ================================================================================================
// FUNCTION  : wiParse_Command()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<command>"  
// Parameters: text    -- Line to parse
//             command -- String/command to match
// ================================================================================================
wiStatus wiParse_Command(wiString text, wiString command)
{
    char *cptr;
    char badchars[] = "~!@#$%^&*()-+=|\\{}[]:;'\"<>,/?\t\n";

    // --------------
    // Error checking
    // --------------
    if (!text) return WI_ERROR_PARAMETER1;
    if (strlen(command) >128) return WI_ERROR_PARAMETER2;
    if (strspn(command, badchars)) return WI_ERROR_PARAMETER2;

    // ----------------------
    // Find the command label
    // ----------------------
    cptr = strstr(text,command);
    if (!cptr) return WI_WARN_PARSE_FAILED;

    // --------------------------------------------
    // Make sure the command label begins the line
    // Spaces (' ') and tabs ('\t') can precede it.
    // --------------------------------------------
    while (text < cptr) 
    { 
        if ((*text == ' ') || (*text == '\t'))
            text++;                           
        else                                 
            return WI_WARN_PARSE_FAILED;
    }
    // ----------------------------
    // Skip any trailing whitespace
    // ----------------------------
    cptr += strlen(command);
    while ((*cptr==' ') || (*cptr=='\t') || (*cptr=='\n'))
        cptr++;

    // ----------------------------------------
    // The next character must be a terminator
    // ---------------------------------------
    if (*cptr != '\0')                // if it is not a '\0'
        return WI_WARN_PARSE_FAILED;  // ...the label is not matched

    return WI_SUCCESS;
}
// end of wiParse_Command()

// ================================================================================================
// FUNCTION  : wiParse_ParamPointer() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a pointer to the a value following a parameter name and equal sign. 
//             The text has the format"<param> = <value>".    
// Parameters: text   -- Line of text to parse 
//             param  -- Parameter label to match 
//             valptr -- Pointer to the value in the string 
// ================================================================================================
wiStatus wiParse_ParamPointer(wiString text, wiString param, char **valptr)
{
    char *cptr;
    char badchars[] = "~!@#$%^&*()-+=|\\{}[]:;'\"<>,/?\t\n";

    // --------------
    // Error checking
    // --------------
    if (!text)                  return WI_ERROR_PARAMETER1;
    if (strlen(param) > 128)    return WI_ERROR_PARAMETER2;
    if (strspn(param,badchars)) return WI_ERROR_PARAMETER2;
    if (!valptr)                return WI_ERROR_PARAMETER3;

    // -------------------------------
    // Default state is a null pointer
    // -------------------------------
    *valptr = NULL;

    // ------------------------
    // Find the parameter label
    // ------------------------
    cptr = strstr(text,param);             // point to the parameter label
    if (!cptr) return WI_WARN_PARSE_FAILED; // if there is no match, return

    // --------------------------------------------
    // Make sure the parameter name begins the line
    // Spaces (' ') and tabs ('\t') can precede it.
    // --------------------------------------------
    while (text < cptr) 
    {
        if ((*text == ' ') || (*text == '\t'))
            text++;                           
        else                                 
            return WI_WARN_PARSE_FAILED;      
    }
    // ----------------------------------------------------------------
    // Make sure that param is not a substring of the current label
    // If the parameter label is followed by ' ' or '=', it is isolated
    // ----------------------------------------------------------------
    cptr += strlen(param);             // move past the parameter label
    if ((*cptr != ' ') && (*cptr != '=') && (*cptr != '\t'))
        return WI_WARN_PARSE_FAILED;

    // ---------------
    // Skip whitespace
    // ---------------
    while ((*cptr==' ') || (*cptr=='\t'))
        cptr++;

    // ----------------------------------
    // The next character must be the '='
    // ----------------------------------
    if (*cptr!='=') return STATUS(WI_ERROR_PARSE_FAILED);
    cptr++; // move past the '='

    // -----------------------
    // Skip any spaces or tabs
    // -----------------------
    while ((*cptr == ' ') || (*cptr == '\t'))
        cptr++;

    // --------------------------------------
    // Now, cptr should point to the value
    // --------------------------------------
    *valptr = cptr;

    return WI_SUCCESS;
}
// end of wiParse_ParamPointer()

// ================================================================================================
// FUNCTION  : wiParse_Boolean()   
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param> = <val>".
// Parameters: text  -- Line of text to parse
//             param -- Parameter label to match  
//             pVal  -- Value returned by reference 
// ================================================================================================
wiStatus wiParse_Boolean(wiString text, wiString param, wiBoolean *pVal)
{
    char *cptr;

    XSTATUS(wiParse_ParamPointer(text, param, &cptr));
    if (!cptr) return WI_WARN_PARSE_FAILED;

    if ((!strcmp(cptr,"FALSE")) || (!strcmp(cptr,"NO"))
    ||  (!strcmp(cptr,"OFF"  )) || (!strcmp(cptr,"0" )))
    {
        if (pVal) *pVal = WI_FALSE;
        return WI_SUCCESS;
    }
    if ((!strcmp(cptr,"TRUE")) || (!strcmp(cptr,"YES"))
    ||  (!strcmp(cptr,"ON"  )) || (!strcmp(cptr,"1"  )))
    {
        if (pVal) *pVal = WI_TRUE;
        return WI_SUCCESS;
    }
    return STATUS(WI_ERROR);
}
// end of wiParse_Boolean()

// ================================================================================================
// FUNCTION  : wiParse_Int()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param> = <val>".
// Parameters: text  -- Line of text to parse
//             param -- Parameter label to match  
//             pVal  -- Value returned by reference 
//             minv  -- Minimum value allowed for the parameter 
//             maxv  -- Maximum value allowed for the parameter
// ================================================================================================
wiStatus wiParse_Int(wiString text, wiString param, wiInt *pVal, wiInt minv, wiInt maxv)
{
    char *cptr, *endptr;
    int ival; double dval;

    if (Verbose) wiPrintf(" wiParse_Int(\"%s\",\"%s\",*)\n",text, param);

    // ---------------------------------
    // Point to the parameter if matched
    // ---------------------------------
    XSTATUS(wiParse_ParamPointer(text, param, &cptr));
    if (!cptr) return WI_WARN_PARSE_FAILED;

    // -----------------------------------------
    // Convert the integer portion to a long int
    // -----------------------------------------
    endptr = cptr;
    if (!wiParse_DoubleValue(&endptr, &dval)) 
        ival = (int)dval;
    else
        ival = (int)strtol(cptr, &endptr, 0);

    if (!wiParse_TextIsWhiteSpace(endptr))
        return WI_ERROR_PARSE_FAILED;

    // --------------
    // Range checking
    // --------------
    if (InvalidRange(ival, minv, maxv))
        return WI_ERROR_PARSE_PARAMETER_RANGE;

    if (pVal) *pVal = ival;

    return WI_SUCCESS;
}
// end of wiParse_Int()

// ================================================================================================
// FUNCTION  : wiParse_UInt() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param> = <val>".
// Parameters: text  -- Line of text to parse
//             param -- Parameter label to match  
//             pVal  -- Value returned by reference 
//             minv  -- Minimum value allowed for the parameter 
//             maxv  -- Maximum value allowed for the parameter
// ================================================================================================
wiStatus wiParse_UInt(wiString text, wiString param, wiUInt *pVal, wiUInt minv, wiUInt maxv)
{
    char  *cptr, *endptr;
    wiUInt uint; double dval;

    if (Verbose) wiPrintf(" wiParse_UInt(\"%s\",\"%s\",*)\n", text, param);

    // ---------------------------------
    // Point to the parameter if matched
    // ---------------------------------
    XSTATUS(wiParse_ParamPointer(text, param, &cptr));
    if (!cptr) return WI_WARN_PARSE_FAILED;

    // -----------------------------------------
    // Convert the integer portion to a long int
    // -----------------------------------------
    endptr = cptr;
    if (!wiParse_DoubleValue(&endptr, &dval)) 
        uint = (wiUInt)dval;
    else
        uint = (wiUInt)strtoul(cptr, &endptr, 0);
    
    if (!wiParse_TextIsWhiteSpace(endptr))
        return WI_ERROR_PARSE_FAILED;

    // --------------
    // Range checking
    // --------------
    if (InvalidRange((wiUInt)uint, minv, maxv)) 
        return WI_ERROR_PARSE_PARAMETER_RANGE;

    if (pVal) *pVal = (wiUInt)uint;

    return WI_SUCCESS;
}
// end of wiParse_UInt()

// ================================================================================================
// FUNCTION  : wiParse_Real()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param> = <val>".
// Parameters: text  -- Line of text to parse
//             param -- Parameter label to match  
//             pVal  -- Value returned by reference 
//             minv  -- Minimum value allowed for the parameter 
//             maxv  -- Maximum value allowed for the parameter
// ================================================================================================
wiStatus wiParse_Real(wiString text, wiString param, wiReal *pVal, wiReal minv, wiReal maxv)
{
    char   *cptr, *endptr;
    double  dval;

    if (Verbose) wiPrintf(" wiParse_Real(\"%s\",\"%s\",*)\n", text, param);

    // ---------------------------------
    // Point to the parameter if matched
    // ---------------------------------
    XSTATUS(wiParse_ParamPointer(text, param, &cptr));
    if (!cptr) return WI_WARN_PARSE_FAILED;

    // ------------------------------------
    // Convert the real portion to a double
    // ------------------------------------
    dval = strtod( cptr, &endptr );

    if (!wiParse_TextIsWhiteSpace(endptr))
        return WI_ERROR_PARSE_FAILED;

    // --------------
    // Range checking
    // --------------
    if (InvalidRange((wiReal)dval, minv, maxv)) 
        return WI_ERROR_PARSE_PARAMETER_RANGE;

    // ----------------
    // Return the value
    // ----------------
    if (pVal) *pVal = (wiReal)dval;

    return WI_SUCCESS;
}
// end of wiParse_Real()

// ================================================================================================
// FUNCTION  : wiParse_String()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param> = <val>".
// Parameters: text   -- Line of text to parse
//             param  -- Parameter label to match  
//             string -- Value returned by reference 
//             maxlen -- Maximum string length 
// ================================================================================================
wiStatus wiParse_String(wiString text, wiString param, char *string, wiUInt maxlen)
{
    char  *cptr;

    if (Verbose) wiPrintf(" wiParse_String(\"%s\",\"%s\",*,%d)\n",text,param,maxlen);

    XSTATUS(wiParse_ParamPointer(text, param, &cptr));
    if (!cptr) return WI_WARN_PARSE_FAILED;

    if (strlen(cptr) >= maxlen )
        return STATUS(WI_ERROR_PARSE_FAILED);

    strcpy(string, cptr);

    return WI_SUCCESS;
}
// end of wiParse_String()

// ================================================================================================
// FUNCTION  : wiParse_ArrayPtr()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param array>[<index>] = <val>".
// Parameters: text     -- Line of text to parse
//             param    -- Parameter label to match
//             maxIndex -- Maximum array index (length is maxIndex + 1)
//             index    -- index value returned by reference
//             valptr   -- pointer to the character string containing the value
// ================================================================================================
wiStatus wiParse_ArrayPtr(wiString text, wiString param, wiInt maxIndex, wiInt *index, wiString *valptr)
{
    char *cptr, *endptr;

    // ------------------------
    // Find the parameter label
    // ------------------------
    cptr = strstr(text, param);
    if (!cptr) return WI_WARN_PARSE_FAILED;

    // ----------------------------------------------------------------------------
    // Make sure the parameter name begins the line (allow leading spaces and tabs)
    // ----------------------------------------------------------------------------
    while (text < cptr) {
        if ((*text==' ') || (*text=='\t')) text++;
        else return WI_WARN_PARSE_FAILED;
    }
    // -------------------------------------------------
    // Make sure the label is followed by a space or '['
    // -------------------------------------------------
    cptr += strlen(param);
    if ((*cptr!=' ') && (*cptr!='[')) return WI_WARN_PARSE_FAILED;
    
    // ----------------------------------------------------
    // Move to the index...past the '[' and skipping spaces
    // ----------------------------------------------------
    while (*cptr==' ') cptr++;
    if (*cptr != '[') return WI_ERROR_PARSE_FAILED;
    cptr++;
    while (*cptr==' ') cptr++;

    // ------------------------
    // Retrieve the index value
    // ------------------------
    *index = (wiInt)strtol(cptr, &endptr, 0);
    if (InvalidRange(*index, 0, maxIndex))
        return WI_ERROR_PARSE_PARAMETER_RANGE;

    // ----------------------------------------------------
    // Move to the "="...past the ']' and skipping spaces
    // ----------------------------------------------------
    cptr = endptr;
    while (*cptr==' ') cptr++;
    if (*cptr != ']') return WI_ERROR_PARSE_FAILED;
    cptr++;
    while (*cptr==' ') cptr++;
    if (*cptr != '=') return WI_ERROR_PARSE_FAILED;
    cptr++;
    while (*cptr==' ') cptr++;

    *valptr = cptr;
    return WI_SUCCESS;
}
// end of wimParse_ArrayPtr()

// ================================================================================================
// FUNCTION  : wiParse_IntArray()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param array>[<index>] = <val>"
// Parameters: text     -- Line of text to parse
//             param    -- Parameter label to match
//             pArray   -- Parameter array pointer
//             maxIndex -- Maximum array index (length is maxIndex + 1)
//             minv     -- Minimum value allowed for the parameter 
//             maxv     -- Maximum value allowed for the parameter
// ================================================================================================
wiStatus wiParse_IntArray(wiString text, wiString param, wiInt *pArray, wiUInt maxIndex,
                                wiInt minv, wiInt maxv)
{
    int index; wiInt val;
    wiString valstr, endptr;
    wiStatus status;

    if (Verbose) wiPrintf(" wiParse_IntArray(\"%s\",\"%s\",*,...)\n",text,param);

    // ------------------------------------
    // Get the array index and value string
    // ------------------------------------
    XSTATUS(status = wiParse_ArrayPtr(text, param, maxIndex, &index, &valstr));
    if (status) return status;

    // --------------------------------------------
    // Convert the value to int and check its range
    // --------------------------------------------
    val = (wiInt)strtol(valstr, &endptr, 0);
    if (InvalidRange(val, minv, maxv)) 
        return WI_ERROR_PARSE_PARAMETER_RANGE;

    // -----------------------------------------
    // Only spaces, tabs, or newlines can follow
    // -----------------------------------------
    while (*endptr) {
        if ((*endptr != ' ') || (*endptr != '\t') || (*endptr != '\n'))
            return WI_ERROR_PARSE_FAILED;
        endptr++;
    }
    // ----------------------------
    // Store the value in the array
    // ----------------------------
    if (pArray) pArray[index] = val;
    return WI_SUCCESS;
}
// end of wiParse_IntArray()

// ================================================================================================
// FUNCTION  : wiParse_UIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param array>[<index>] = <val>"
// Parameters: text     -- Line of text to parse
//             param    -- Parameter label to match
//             pArray   -- Parameter array pointer
//             maxIndex -- Maximum array index (length is maxIndex + 1)
//             minv     -- Minimum value allowed for the parameter 
//             maxv     -- Maximum value allowed for the parameter
// ================================================================================================
wiStatus wiParse_UIntArray(wiString text, wiString param, wiUInt *pArray, wiUInt maxIndex, wiUInt minv, wiUInt maxv)
{
    int index; wiUInt val; double dval;
    wiString valstr, endptr;
    wiStatus status;

    if (Verbose) wiPrintf(" wiParse_UIntArray(\"%s\",\"%s\",*,...)\n",text,param);

    // ------------------------------------
    // Get the array index and value string
    // ------------------------------------
    XSTATUS( status = wiParse_ArrayPtr(text, param, maxIndex, &index, &valstr) );
    if (status) return status;

    // ---------------------------------------------
    // Convert the value to uint and check its range
    // ---------------------------------------------
    endptr = valstr;
    if (!wiParse_DoubleValue(&endptr, &dval)) 
        val = (wiUInt)dval;
    else
        val = (wiUInt)strtoul(valstr, &endptr, 0);
    
    if (InvalidRange(val, minv, maxv)) 
        return WI_ERROR_PARSE_PARAMETER_RANGE;

    // -----------------------------------------
    // Only spaces, tabs, or newlines can follow
    // -----------------------------------------
    while (*endptr) 
    {
        if ((*endptr != ' ') || (*endptr != '\t') || (*endptr != '\n'))
            return WI_ERROR_PARSE_FAILED;
        endptr++;
    }
    // ----------------------------
    // Store the value in the array
    // ----------------------------
    if (pArray) pArray[index] = val;
    return WI_SUCCESS;
}
// end of wiParse_UIntArray()

// ================================================================================================
// FUNCTION  : wiParse_RealArray()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text of the form "<param array>[<index>] = <val>"
// Parameters: text     -- Line of text to parse
//             param    -- Parameter label to match
//             pArray   -- Parameter array pointer
//             maxIndex -- Maximum array index (length is maxIndex + 1)
//             minv     -- Minimum value allowed for the parameter 
//             maxv     -- Maximum value allowed for the parameter
// ================================================================================================
wiStatus wiParse_RealArray(wiString text, wiString param, wiReal *pArray, wiUInt maxIndex, wiReal minv, wiReal maxv)
{
    int index; wiReal val;
    wiString valstr, endptr;
    wiStatus status;

    if (Verbose) wiPrintf(" wiParse_RealArray(\"%s\",\"%s\",*,...)\n",text,param);

    // ------------------------------------
    // Get the array index and value string
    // ------------------------------------
    XSTATUS(status = wiParse_ArrayPtr(text, param, maxIndex, &index, &valstr));
    if (status) return status;

    // ---------------------------------------------
    // Convert the value to real and check its range
    // ---------------------------------------------
    val = strtod(valstr, &endptr);
    if (InvalidRange(val, minv, maxv)) 
        return WI_ERROR_PARSE_PARAMETER_RANGE;

    // -----------------------------------------
    // Only spaces, tabs, or newlines can follow
    // -----------------------------------------
    while (*endptr) {
        if ((*endptr != ' ') || (*endptr != '\t') || (*endptr != '\n'))
            return WI_ERROR_PARSE_FAILED;
        endptr++;
    }
    // ----------------------------
    // Store the value in the array
    // ----------------------------
    if (pArray) pArray[index] = val;
    return WI_SUCCESS;
}
// end of wiParse_RealArray()

// ================================================================================================
// FUNCTION  : simParse_Function()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a set of values from a script line with the form
//                 <function>(<value 1>,<value 2>,...,<value n>) 
// Parameters: text   -- Line of text to parse
//             format -- Format for parameters inside function parenthesis
//             ...    -- Variable argument list depends on format field
// ================================================================================================
wiStatus wiParse_Function(wiString text, wiString format, ...)
{
    char   *cptr, *endptr, label[96];
    double dval; long lval; void *ptr;  int i = 0, n = 0;
    va_list ap;

    if (Verbose) wiPrintf(" wiParse_Function(\"%s\",\"%s\",...)\n", text, format);

    // -------------------------------
    // Initialize the argument pointer
    // -------------------------------
    va_start(ap, format);

    // -------------------------------------------------------
    // Verify that text and format have the same basic form
    // -------------------------------------------------------
    strncpy(label, format, 96);
    label[95] = '\0';

    cptr = strchr(label,'(');
    if (!cptr) return STATUS(WI_ERROR_PARAMETER2);

    cptr = strchr(cptr,')');
    if (!cptr) return STATUS(WI_ERROR_PARAMETER2);

    *strchr(label,'(') = '\0';

    cptr = strstr(text,label);
    if (cptr != text) return WI_WARN_PARSE_FAILED;
                            
    text += strlen(label) + 1;
    format += strlen(label) + 1;

    // ---------------------------------
    // Check the number of arguments (%)
    // ---------------------------------
    cptr = format;
    for (n=0; strchr(cptr,'%'); n++)
        cptr = strchr(cptr,'%') + 1;
    if (!n) return STATUS(WI_ERROR_PARAMETER1);

    cptr = format;
    for (i=0; strchr(cptr,','); i++)
        cptr = strchr(cptr,',') + 1;
    if (n != (i+1)) return STATUS(WI_ERROR_PARAMETER2);

    cptr = text;
    for (i=0; strchr(cptr,','); i++)
        cptr = strchr(cptr,',') + 1;
    if (n != (i+1)) return STATUS(WI_ERROR_PARAMETER1);

    // --------------------
    // Parse the parameters
    // --------------------
    while (strchr(format,'%'))
    {
        while (*text   == ' ') text++;
        while (*format == ' ') format++;

        if (*format != '%') {
            va_end(ap);
            return STATUS(WI_ERROR_PARAMETER2);
        }
        format++;

        // -----------------------------
        // Get the next argument pointer
        // -----------------------------
        ptr = va_arg(ap, void *);
        if (!ptr && *format != 'v') {
            va_end(ap);
            return STATUS(WI_ERROR_PARAMETER2);
        }

        // -------------
        // Parse by type
        // -------------
        switch (*format)
        {
            case 'd':
            case 'i':
                endptr = text;
                if (!wiParse_DoubleValue(&endptr, &dval)) 
                    *((int *)ptr) = (int)dval;
                else
                    *((int *)ptr) = (int)strtol(text,&endptr,0);
                break;

            case 'f':
                *((wiReal *)ptr) = (wiReal)strtod(text, &endptr);;
                break;

            case 'v':
                for (endptr=text+1; (*endptr && (*endptr!=',') && (*endptr!=')')); endptr++) ;
                break;

            default:
                lval = strtol(format,&endptr,0);
                if (InvalidRange(lval,1,32767)) return STATUS(WI_ERROR_PARAMETER2);
                if (*endptr=='s')
                {
                    if (*text != '"') {
                        va_end(ap); return STATUS(WI_ERROR_PARAMETER1);
                    }
                    text++;
                    strncpy((char *)ptr, text, lval);
                    ((char *)ptr)[lval-1] = '\0';

                    cptr = strchr((char *)ptr,'"');
                    if (cptr) *cptr = '\0';

                    text += (strlen((char *)ptr) + 1);
                    format = endptr;  endptr = text;
                }
                else
                {
                    // -----------------------------
                    // Unrecognized format specifier
                    // -----------------------------
                    va_end(ap);
                    return STATUS(WI_ERROR_PARAMETER2);
                }
        }
        // ---------------------
        // Check the end pointer
        // ---------------------
        while (*endptr == ' ') endptr++;
        if ((*endptr != ',') && (*endptr != ')')) {
            va_end(ap); return STATUS(WI_ERROR_PARSE_FAILED);
        }
        // -------------------------------------------------------------
        // Advance pointers in the format and text to the next parameter
        // -------------------------------------------------------------
        while ((*endptr==' ') || (*endptr=='\t')) endptr++;
        if (*endptr==',') endptr++;

        while ((*endptr==' ') || (*endptr=='\t')) endptr++;
        text = endptr;

        format++;
        while ((*format==' ') || (*format=='\t')) format++;
        if (*format==',') format++;

        while ((*format==' ') || (*format=='\t')) format++;
    }
    // ----------------------------------------
    // Clean up the argument pointer and return
    // ----------------------------------------
    va_end(ap);

    return WI_SUCCESS;
}
// end of wiParse_Function()

// ================================================================================================
// FUNCTION  : wiParse_SubstituteStrings()
// ------------------------------------------------------------------------------------------------
// Purpose   : Perform string substitutions based on those loaded via previous script commands
// ================================================================================================
wiStatus wiParse_SubstituteStrings(wiString *text)
{
    char *cptr;
    char *cptr0 = *text;
    int n;

    while (*cptr0 == ' ' || *cptr0 == '\t') cptr0++;

    for (n=0; n<WIPARSE_MAX_SUBSTITUTIONS; n++)
    {
        if (pState->Substitute.OldName[n][0] == '\0') continue; // no entry                        

        cptr = cptr0;

        if (cptr == strstr(cptr, pState->Substitute.OldName[n]))
        {
            cptr += strlen(pState->Substitute.OldName[n]);

            if (*cptr == '.' || *cptr == '_')
            {
                sprintf(pState->Substitute.NewLine,"%s%s", pState->Substitute.NewName[n], cptr);
                *text = pState->Substitute.NewLine;
            }
        }
    }
    return WI_SUCCESS;
}
// end of wiParse_SubstituteStrings()    

// ================================================================================================
// FUNCTION  : wiParse_Line()  
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiParse_Line(wiString text)
{
    wiParseMethod_t *method;
    char buf[256], buf2[32];  int n;
    wiStatus pstatus, status;

    if (Verbose)      wiPrintf(" wiParse_Line(\"%s\")\n",text);
    if (pState->Echo) wiPrintf(" wiParse> %s\n",text);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set the Verbose or Echo Mode
    //
    PSTATUS(wiParse_Boolean(text,"wiParse.Verbose",&Verbose) );
    PSTATUS(wiParse_Boolean(text,"wiParse.Echo",&(pState->Echo)) );

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Parse an Included File
    //
    if (wiParse_Function(text,"wiParse_ScriptFile(%255s)",buf) == WI_SUCCESS) {
        XSTATUS(wiParse_ScriptFile(buf));
        return WI_SUCCESS;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Pause or Exit a Script File
    //
    if ((wiParse_Command(text, "Exit()") == WI_SUCCESS) || (wiParse_Command(text, "exit()") == WI_SUCCESS))
    {
        XSTATUS( wiParse_ExitScript() );
        return WI_SUCCESS;
    }
    if ((wiParse_Command(text,"Pause()") == WI_SUCCESS) || (wiParse_Command(text,"pause()") == WI_SUCCESS))
    {
        XSTATUS( wiParse_PauseScript() );
        return WI_SUCCESS;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Execute a System Command
    //
    if (wiParse_Function(text,"system(%255s)",buf) == WI_SUCCESS) {
        int rc = system(buf);
        if (rc) wiUtil_WriteLogFile("Return Code = %d for command (on next line)\n   system(\"%s\")\n",buf);
        return WI_SUCCESS;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Modify the Path Options for wiParse_OpenFile()
    //
    //  When wiParse_OpenFile() is used in place of fopen(), alternate search paths
    //  (in addition to the current directory) may be specfied. The functions below add
    //  or remove a path from the list.
    //
    if (wiParse_Function(text,"wiParse_AddPath(%255s)",buf) == WI_SUCCESS)
    {
        for (n=0; n<WIPARSE_MAX_FILEPATHS; n++)
        {
            if (pState->FilePath[n]) continue;
            if (strlen(buf) > 0)
            {
                WICALLOC( pState->FilePath[n], strlen(buf)+1, char );
                strncpy(pState->FilePath[n], buf, strlen(buf));
                return WI_SUCCESS;
            }
            else return STATUS(WI_ERROR_PARAMETER1);
        }
        wiUtil_WriteLogFile("Maximum number of paths already set\n");
        return WI_ERROR;
    }
    if (wiParse_Function(text,"wiParse_RemovePath(%255s)",buf) == WI_SUCCESS)
    {
        for (n=0; n<WIPARSE_MAX_FILEPATHS; n++)
        {
            if (pState->FilePath[n])
            {
                if (strcmp(buf, pState->FilePath[n]) == 0)
                {
                    wiFree(pState->FilePath[n]);
                    pState->FilePath[n] = NULL;
                }
            }
        }
        return WI_SUCCESS;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Assign a random value to an integer or real parameter.
    //
    //  These functions can be used as part of a Monte Carlo or similar simulation
    //  that requires randomized parametrics.
    {
        char cmd[512]; wiInt x1, x2;
        XSTATUS(status = wiParse_Function(text,"SetRndInt(%255s,%d,%d)",buf,&x1,&x2));
        if (status==WI_SUCCESS)
        {
            wiReal dx = (double)(x2 - x1);
            wiReal x = x1 + (int)(dx * wiRnd_UniformSample());
            sprintf(cmd,"%s = %d",buf,(int)x);
            XSTATUS(wiParse_Line(cmd));
            return WI_SUCCESS;
        }
    }
    //  Real Parameter
    {
        char cmd[512]; wiReal x1, x2;
        XSTATUS(status = wiParse_Function(text,"SetRndReal(%255s,%f,%f)",buf,&x1,&x2));
        if (status==WI_SUCCESS)
        {
            wiReal dx = x2-x1;
            wiReal x = x1 + dx * wiRnd_UniformSample();
            sprintf(cmd,"%s = %1.16e",buf,x);
            XSTATUS(wiParse_Line(cmd));
            return WI_SUCCESS;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Store/Retrieve General Purpose Parameters
    //
    //  The integer and real parameters provide general purpose storage for values that
    //  can later be assigned to any parsable parameter. This allows a script to be
    //  written with parameters assigned previously.
    //
    PSTATUS( wiParse_IntArray (text, "wiParse.IntParameter",  pState->IntParameter,  WIPARSE_MAX_PARAMETERS-1,  INT_MIN, INT_MAX) );
    PSTATUS( wiParse_RealArray(text, "wiParse.RealParameter", pState->RealParameter, WIPARSE_MAX_PARAMETERS-1, -FLT_MAX, FLT_MAX) );

    XSTATUS( status = wiParse_Function(text,"wiParse_SetIntParameter(%d, %255s)",&n, buf) );
    if (status == WI_SUCCESS)
    {
        char NewText[512];
        if (InvalidRange(n, 0, WIPARSE_MAX_PARAMETERS-1)) return WI_ERROR_RANGE;
        sprintf(NewText, "%s = %d",buf, pState->IntParameter[n]);
        wiUtil_WriteLogFile("wiParse: %s\n",NewText);
        status = STATUS(wiParse_Line(NewText));
        return (status == WI_SUCCESS) ? WI_SUCCESS : STATUS(WI_ERROR);
    }
    XSTATUS( status = wiParse_Function(text,"wiParse_SetRealParameter(%d, %255s)",&n, buf) );
    if (status == WI_SUCCESS)
    {
        char NewText[512];
        if (InvalidRange(n, 0, WIPARSE_MAX_PARAMETERS-1)) return WI_ERROR_RANGE;
        sprintf(NewText, "%s = %1.12e",buf, pState->RealParameter[n]);
        wiUtil_WriteLogFile("wiParse: %s\n",NewText);
        status = STATUS(wiParse_Line(NewText));
        return (status == WI_SUCCESS) ? WI_SUCCESS : STATUS(WI_ERROR);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set the priority class. The process priority can be set to either idle (low) or
    //  normal on a Windows machine. For other OS types, the commands are ignored.
    //
    if (wiParse_Command(text,"SetPriorityClass(IDLE)") == WI_SUCCESS) {
        XSTATUS( wiProcess_SetPriority(WIPROCESS_PRIORITY_IDLE) );
        return WI_SUCCESS;
    }
    if (wiParse_Command(text,"SetPriorityClass(NORMAL)") == WI_SUCCESS) {
        XSTATUS( wiProcess_SetPriority(WIPROCESS_PRIORITY_NORMAL) );
        return WI_SUCCESS;
    }

    // Display a message on the console
    // 
    if (wiParse_Function(text,"Display(%255s)",buf) == WI_SUCCESS) { wiPrintf("%s\n",buf); return WI_SUCCESS; }
    if (wiParse_Function(text,"display(%255s)",buf) == WI_SUCCESS) { wiPrintf("%s\n",buf); return WI_SUCCESS; }
    if (wiParse_Function(text,"disp(%255s)",   buf) == WI_SUCCESS) { wiPrintf("%s\n",buf); return WI_SUCCESS; }

    // Store Name Substitutions
    //
    XSTATUS( status = wiParse_Function(text, "wiParse_Substitute(%31s, %31s)", buf, buf2) );
    if (status == WI_SUCCESS)
    {
        for (n=0; n<WIPARSE_MAX_SUBSTITUTIONS; n++)
            if (0 == strcmp(pState->Substitute.OldName[n], buf)) break; // matching entry

        if (n == WIPARSE_MAX_SUBSTITUTIONS)
        {
            for (n=0; n<WIPARSE_MAX_SUBSTITUTIONS; n++)
                if (pState->Substitute.OldName[n][0] == '\0') break; // empty slot
        }
        if (n == WIPARSE_MAX_SUBSTITUTIONS) return STATUS(WI_ERROR); // no entries available

        strcpy( pState->Substitute.OldName[n], buf );
        strcpy( pState->Substitute.NewName[n], buf2);
        if (buf2[0] == '\0') pState->Substitute.OldName[n][0] = '\0'; // remove entry

        return WI_SUCCESS;            
    }

    // Perform Name Substitutions
    //
    XSTATUS( wiParse_SubstituteStrings(&text) );

    //  Call the external script parsers specified with wiParse_AddMethod()
    //
    for (method = pState->MethodList; method; method = method->next) {
        PSTATUS( (method->fn)(text) );
    }

    //  No parsers could process the line so handle it as an error
    //
    {
        char *cptr;

        for (cptr = text + strlen(text) - 1; cptr > text; cptr--) 
        {
            if ((*cptr==' ') || (*cptr=='\t') || (*cptr=='\n'))
                *cptr = '\0';
            else break;
        }
        wiUtil_WriteLogFile(" Error parsing text = \"%s\"\n", text);
    }
    return WI_WARN_PARSE_FAILED;
}
// end of wiParse_Line()

// ================================================================================================
// FUNCTION  : wiParse_TextIsWhiteSpace() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns true if the line of text contains only whitespace
// Parameters: cptr -- Pointer to a character string    
// ================================================================================================
wiBoolean wiParse_TextIsWhiteSpace(char *cptr)
{
    while (*cptr) 
    {
        if ( (*cptr != ' ') || (*cptr != '\t') || (*cptr != '\n') ) 
            return WI_FALSE;

        cptr++;
    }
    return WI_TRUE;
}
// end of wiParse_TextIsWhiteSpace()

// ================================================================================================
// FUNCTION  : wiParse_DoubleValue()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns the double value from an exponential notion in the string pointer to by cptr.
//             The value of cptr is advanced to the end of the value in the string.
// Parameters: pptr -- Pointer to a string pointer
// ================================================================================================
wiStatus wiParse_DoubleValue(char **pptr, double *dval)
{
    char *cptr = *pptr;

    if (strstr(cptr,"E+") || strstr(cptr,"E-") || strstr(cptr,"e+") || strstr(cptr,"e-") 
        || (strstr(cptr,"e") && (strstr(cptr,".") || !(strstr(cptr,"0x") || strstr(cptr,"0X")))))
    {
        *dval = strtod(cptr, pptr);
        return WI_SUCCESS;
    }
    return WI_WARN_PARSE_FAILED;
}
// end of wiParse_DoubleValue()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// Revisions_______________________________________________________________________________________

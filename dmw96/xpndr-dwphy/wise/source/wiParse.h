//
//  wiParse -- Script file parsing utilities
//  Copyright 2009 DSP Group, Inc. All rights reserved.
//     also copyright 2001-2002 Bermai, Inc. All rights reserved.
//     also Copyright 1995-1997 Barrett Brickner. All rights reserved.
//
//  The script parsing functions provided in this module are used to provide a convenient 
//  interface for runtime modification to program parameters when compiled as a console
//  executable. The script file is plain text and can be modified to change parameters
//  such as SNR limits and delay spreads in the channel as well as code rates or other
//  transceiver operating parameters.
//
//  The mechanism for automatically parsing a script file is coded into wiMain. The users's
//  responsibility is for adding individual line parsing functions to account for parameters
//  and options defined within modules they add to this simulator. For an example of how this
//  code is used, see the wiParse_AddMethod function call in wiTest_Init and the individual 
//  parameter assignments and test routine execution in wiTest_ParseLine.
//

#ifndef __WIPARSE_H
#define __WIPARSE_H

#include <stdio.h> // required for FILE definition
#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//
//  Define Limits
//
//  WIPARSE_MAX_LINE_LENGTH     -- Maximum text line length (characters)
//  WIPARSE_MAX_RECURSION_DEPTH -- Maximum number of nested files
//  WIPARSE_MAX_PARAMETERS      -- Number of generic wiParse.Parameter values
//
#define WIPARSE_MAX_LINE_LENGTH     1024
#define WIPARSE_MAX_RECURSION_DEPTH   10
#define WIPARSE_MAX_PARAMETERS        10
#define WIPARSE_MAX_NAME_LENGTH       32
#define WIPARSE_MAX_SUBSTITUTIONS     16
#define WIPARSE_MAX_FILEPATHS         16
#define WIPARSE_MAX_FILENAME_LENGTH  512

/////////////////////////////////////////////////////////////////////////////////////////
//
//  Type Definitions
//
//  wiParseFunction_t is a function pointer for text parsing methods
//  wiParse_Method_t is a linked-list element to record method functions
//  wiParse_Substitution_t provides a list of substitution strings
//
typedef wiStatus (*wiParseFunction_t)(wiString);

typedef struct wiParseMethodS 
{
    wiParseFunction_t      fn;   // pointer to the text line parsing function
    struct wiParseMethodS *next; // pointer to the next record in the linked list
    struct wiParseMethodS *prev; // pointer to the previous record in the linked list

} wiParseMethod_t;

/////////////////////////////////////////////////////////////////////////////////////////
//
//  Module State Structure
//
typedef struct
{
    wiBoolean Echo;
    wiBoolean AbortOnError;
    wiBoolean ExitScript;
    wiBoolean PauseScript;
    wiBoolean ScriptIsPaused;
    int       RecursionDepth;
    
    int       PausedLine;
    FILE     *PausedFile;
    char     *PausedFileName;

    char     *FilePath[WIPARSE_MAX_FILEPATHS];
    char      LastFileName[WIPARSE_MAX_FILENAME_LENGTH];
    
    wiInt     IntParameter [WIPARSE_MAX_PARAMETERS];
    wiReal    RealParameter[WIPARSE_MAX_PARAMETERS];

    wiParseMethod_t *MethodList;

    struct
    {
        char OldName[WIPARSE_MAX_SUBSTITUTIONS][WIPARSE_MAX_NAME_LENGTH];
        char NewName[WIPARSE_MAX_SUBSTITUTIONS][WIPARSE_MAX_NAME_LENGTH];
        char NewLine[WIPARSE_MAX_LINE_LENGTH + WIPARSE_MAX_NAME_LENGTH];

    } Substitute;
}
wiParse_State_t;

/////////////////////////////////////////////////////////////////////////////////////////
//
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//
wiStatus wiParse_Init(void);
wiStatus wiParse_Close(void);

wiStatus wiParse_AddMethod(wiParseFunction_t fn);
wiStatus wiParse_RemoveMethod(wiParseFunction_t fn);

wiStatus wiParse_CleanLine(wiString InString, char *OutString, wiUInt MaxLength);
wiStatus wiParse_ParamPointer(wiString text, wiString param, wiString *ParamPtr );

wiStatus wiParse_SubstituteStrings(wiString *text);
FILE *   wiParse_OpenFile(wiString filename, wiString mode);

//---- Script File Control ------------------------------------------------------------------------
wiStatus  wiParse_ExitScript(void);
wiStatus  wiParse_PauseScript(void);
wiStatus  wiParse_ResumeScript(void);
wiBoolean wiParse_ScriptIsPaused(void);

//---- Parse a Script File or Line of Text --------------------------------------------------------
wiStatus wiParse_ScriptFile(wiString Filename);
wiStatus wiParse_Line      (wiString text);

//---- Parse Single Parameters, <Label> = <Value> -------------------------------------------------
wiStatus wiParse_Boolean (wiString text, wiString param, wiBoolean *val);
wiStatus wiParse_Int     (wiString text, wiString param, wiInt     *val, wiInt  minv, wiInt  maxv);
wiStatus wiParse_UInt    (wiString text, wiString param, wiUInt    *val, wiUInt minv, wiUInt maxv);
wiStatus wiParse_Real    (wiString text, wiString param, wiReal    *val, wiReal minv, wiReal maxv);
wiStatus wiParse_String  (wiString text, wiString param, char      *val, wiUInt maxlength);

//---- Parse More Complicated Syntax --------------------------------------------------------------
wiStatus wiParse_IntArray (wiString text, wiString param, wiInt  *pArray, wiUInt maxIndex, wiInt  minv, wiInt  maxv);
wiStatus wiParse_UIntArray(wiString text, wiString param, wiUInt *pArray, wiUInt maxIndex, wiUInt minv, wiUInt maxv);
wiStatus wiParse_RealArray(wiString text, wiString param, wiReal *pArray, wiUInt maxIndex, wiReal minv, wiReal maxv);
wiStatus wiParse_Function (wiString text, wiString format, ...);

//---- Commands and Other Script Constructions  ---------------------------------------------------
wiStatus wiParse_Command (wiString text, wiString CommandStr);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

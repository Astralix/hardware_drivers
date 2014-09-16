//
//  wiError.h -- Status/Error codes for WISE
//  Copyright 2001 Bermai, 2006-2011 DSP Group, Inc. All rights reserved.
//  
//  The status codes defined in this module are those values expected for return values of 
//  type wiStatus. WI_SUCCESS is returned when there is not error to report; the remainder 
//  define various warning and error conditions.
//

#ifndef __WIERROR_H
#define __WIERROR_H

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Status Code Offset
//  Not really important, but selected to be different from VISA and DwPhy offsets which are
//  constructed similarly.
//
#define WI_STATUS_OFFSET 0x011A0000L

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Enumeration of Status Codes
//  Successful functions return a zero (0) status code. Errors produce a negative number.
//  Warnings and other return status values are positive. All offsets added to WI_WARNING and 
//  WI_ERROR must have values in the range 0x0000 to 0xFFFF.
//
enum wiErrorE
{
    WI_SUCCESS = 0, // Normal Status

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  WARNINGS
    //  Warnings are informational values that indicate a potential problem but may not
    //  require attention by the calling function. Typically, they indicate the absence
    //  of a particular feature, without which execution can still continue.
    //
    WI_WARNING                          = (WI_STATUS_OFFSET + 0x40000000L),
    WI_WARN_UNKNOWN_STATUS              = (WI_WARNING + 0x0001L),
    WI_WARN_END_OF_FILE                 = (WI_WARNING + 0x0002L),
    WI_WARN_BUFFER_OVERFLOW             = (WI_WARNING + 0x0003L),
    WI_WARN_FUNCTION_UNAVAILABLE        = (WI_WARNING + 0x0004L),
    WI_WARN_LOG_FILE_DISABLED           = (WI_WARNING + 0x0005L),
    WI_WARN_WINDOWS_ONLY                = (WI_WARNING + 0x0010L),
    WI_WARN_WINDOWS_NT_ONLY             = (WI_WARNING + 0x0011L),
    WI_WARN_PARSE_FAILED                = (WI_WARNING + 0x0050L),
    WI_WARN_MODULE_NOT_INITIALIZED      = (WI_WARNING + 0x0100L),
    WI_WARN_MODULE_ALREADY_INIT         = (WI_WARNING + 0x0101L),
    WI_WARN_METHOD_UNHANDLED_PARAMETER  = (WI_WARNING + 0x0118L),
    WI_WARN_METHOD_UNHANDLED_COMMAND    = (WI_WARNING + 0x0119L),

    //////  MODEM OPERATION  ////////////////////////////////////////////////////////////
    WI_WARN_PHY_SIGNAL_PARITY           = (WI_WARNING + 0x1001L),
    WI_WARN_PHY_SIGNAL_RATE             = (WI_WARNING + 0x1002L),
    WI_WARN_PHY_SIGNAL_LENGTH           = (WI_WARNING + 0x1003L),
    WI_WARN_PHY_BAD_PACKET              = (WI_WARNING + 0x1004L),

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  ERRORS
    //  The following are errors which when returned by a function should be handled by
    //  the calling function. They represent a critical problem and indicate that 
    //  execution should not continue.
    //
    WI_ERROR                            = (WI_STATUS_OFFSET - 2147483647L - 1),
    WI_ERROR_MEMORY_ALLOCATION          = (WI_ERROR + 0x0009L),

    WI_ERROR_FILE_OPERATION             = (WI_ERROR + 0x0010L),
    WI_ERROR_FILE_OPEN                  = (WI_ERROR + 0x0011L),
    WI_ERROR_FILE_WRITE                 = (WI_ERROR + 0x0012L),
    WI_ERROR_FILE_READ                  = (WI_ERROR + 0x0013L),
    WI_ERROR_FILE_FORMAT                = (WI_ERROR + 0x0014L),

    WI_ERROR_RETURNED                   = (WI_ERROR + 0x0020L),

    WI_ERROR_FUNCTION_UNAVAILABLE       = (WI_ERROR + 0x0031L),
    WI_ERROR_UNDEFINED_CASE             = (WI_ERROR + 0x0032L),

    WI_ERROR_DIVIDE_BY_ZERO             = (WI_ERROR + 0x0040L),

    WI_ERROR_PARSE_FAILED               = (WI_ERROR + 0x0050L),
    WI_ERROR_PARSE_PARAMETER_RANGE      = (WI_ERROR + 0x0051L),

    WI_ERROR_MODULE_NOT_INITIALIZED     = (WI_ERROR + 0x0100L),
    WI_ERROR_MODULE_ALREADY_INIT        = (WI_ERROR + 0x0101L),
    WI_ERROR_INITIALIZING_MODULE        = (WI_ERROR + 0x0102L),

    WI_ERROR_INVALID_METHOD             = (WI_ERROR + 0x0110L),
    WI_ERROR_METHOD_COMMAND             = (WI_ERROR + 0x0111L),
    WI_ERROR_METHOD_PARAM               = (WI_ERROR + 0x0112L),
    WI_ERROR_METHOD_PARAM_RANGE         = (WI_ERROR + 0x0113L),
    WI_ERROR_METHOD_UNHANDLED_COMMAND   = (WI_ERROR + 0x0119L),

    WI_ERROR_RANGE                      = (WI_ERROR + 0x0200L),
    WI_ERROR_PARAMETER1                 = (WI_ERROR + 0x0201L),
    WI_ERROR_PARAMETER2                 = (WI_ERROR + 0x0202L),
    WI_ERROR_PARAMETER3                 = (WI_ERROR + 0x2003L),
    WI_ERROR_PARAMETER4                 = (WI_ERROR + 0x0204L),
    WI_ERROR_PARAMETER5                 = (WI_ERROR + 0x0205L),
    WI_ERROR_PARAMETER6                 = (WI_ERROR + 0x0206L),
    WI_ERROR_PARAMETER7                 = (WI_ERROR + 0x0207L),
    WI_ERROR_PARAMETER8                 = (WI_ERROR + 0x0208L),
    WI_ERROR_PARAMETER9                 = (WI_ERROR + 0x0209L),
    WI_ERROR_PARAMETER10                = (WI_ERROR + 0x020AL),
    WI_ERROR_PARAMETER11                = (WI_ERROR + 0x020BL),
    WI_ERROR_PARAMETER12                = (WI_ERROR + 0x020CL),
    WI_ERROR_PARAMETER13                = (WI_ERROR + 0x020DL),
    WI_ERROR_PARAMETER14                = (WI_ERROR + 0x020EL),
    WI_ERROR_PARAMETER15                = (WI_ERROR + 0x020FL),
    WI_ERROR_PARAMETER16                = (WI_ERROR + 0x0210L),
    WI_ERROR_PARAMETER17                = (WI_ERROR + 0x0211L),
    WI_ERROR_PARAMETER18                = (WI_ERROR + 0x0212L),
    WI_ERROR_PARAMETER19                = (WI_ERROR + 0x0213L),
    WI_ERROR_PARAMETER20                = (WI_ERROR + 0x0214L),
    WI_ERROR_MATRIX_SIZE_MISMATCH       = (WI_ERROR + 0x0220L),
    WI_ERROR_MATRIX_INDEX_OUT_OF_RANGE  = (WI_ERROR + 0x0221L),

    WI_ERROR_WINDOWS_ONLY               = (WI_ERROR + 0x0310L),
    WI_ERROR_WINDOWS_NT_ONLY            = (WI_ERROR + 0x0311L),
    WI_ERROR_INVALID_FOR_THREAD         = (WI_ERROR + 0x0320L),
    WI_ERROR_NO_MULTITHREAD_SUPPORT     = (WI_ERROR + 0x0321L),
};

//-------------------------------------------------------------------------------------------------
#endif // __WIERROR_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

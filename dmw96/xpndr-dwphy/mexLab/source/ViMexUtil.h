//
// ViMexUtil.h -- Matlab Executable Utility Functions for VISA
//
// Copyright 2007 DSP Group, Inc. All rights reserved.
//    This module is derived from wiMexUtil (WiSE)

#ifndef __VIMEXUTIL_H
#define __VIMEXUTIL_H

#include "mex.h"   // include Matlab data types
#include "visa.h"  // include Visa data types

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// ================================================================================================
// USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
// ================================================================================================
void      ViMexUtil_CheckParameters(int nlhs, int nrhs, int nlhs_req, int nrhs_req);

//-------- Get Values/Arrays from MATLAB ----------------------------------------------------------
ViString   ViMexUtil_GetStringValue  (const mxArray *prhs, int *length);
int        ViMexUtil_GetIntValue     (const mxArray *prhs);
ViUInt32   ViMexUtil_GetUIntValue    (const mxArray *prhs);
ViReal64   ViMexUtil_GetRealValue    (const mxArray *prhs);
int      * ViMexUtil_GetIntArray     (const mxArray *prhs, int *length);
ViUInt8  * ViMexUtil_GetBinaryArray(const mxArray *prhs, int *length);
ViReal64 * ViMexUtil_GetRealArray    (const mxArray *prhs, int *length);

//-------- Put Values/Arrays into MATLAB ----------------------------------------------------------
void ViMexUtil_PutIntValue    (int       x, mxArray **plhs);
void ViMexUtil_PutUIntValue   (ViUInt32  x, mxArray **plhs);
void ViMexUtil_PutRealValue   (ViReal64  x, mxArray **plhs);
void ViMexUtil_PutBooleanValue(ViBoolean x, mxArray **plhs);
void ViMexUtil_PutString      (ViString  x, mxArray **plhs);

void ViMexUtil_PutIntArray   (int n, int      *x, mxArray **plhs);
void ViMexUtil_PutRealArray  (int n, ViReal64 *x, mxArray **plhs);
void ViMexUtil_PutBinaryArray(int n, void     *x, mxArray **plhs);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

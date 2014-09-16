//
// wiMexUtil.h -- Matlab Executable Utility Functions for WiSE
//
// Copyright 2001 Bermai, 2007 DSP Group, Inc. All rights reserved.
//    also Copyright 1998 Barrett Brickner. All rights reserved.

#ifndef __WIMEXUTIL_H
#define __WIMEXUTIL_H

#include "mex.h"    // include Matlab data types
#include "wiType.h" // include WiSE data types

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// ================================================================================================
// USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
// ================================================================================================
void       WIAPI wiMexUtil_CheckParameters(int nlhs, int nrhs, int nlhs_req, int nrhs_req);
wiStatus   CFUNC wiMexUtil_Output(char *format, ...);
void       WIAPI wiMexUtil_ResetMsgFunc(void);
wiStatus __cdecl wiMexUtil_MsgFunc(const char *Fmt, ...);
wiString   WIAPI wiMexUtil_GetMsgFuncBuffer(void);

//-------- Get Values/Arrays from MATLAB ----------------------------------------------------------
wiString    WIAPI wiMexUtil_GetStringValue  (const mxArray *prhs, int *length);
int         WIAPI wiMexUtil_GetIntValue     (const mxArray *prhs);
wiUInt      WIAPI wiMexUtil_GetUIntValue    (const mxArray *prhs);
wiReal      WIAPI wiMexUtil_GetRealValue    (const mxArray *prhs);
wiBoolean   WIAPI wiMexUtil_GetBooleanValue (const mxArray *prhs);
int        *WIAPI wiMexUtil_GetIntArray     (const mxArray *prhs, int *length);
wiReal     *WIAPI wiMexUtil_GetRealArray    (const mxArray *prhs, int *length);
wiComplex  *WIAPI wiMexUtil_GetComplexArray (const mxArray *prhs, int *length);
wiComplex **WIAPI wiMexUtil_GetComplexMatrix(const mxArray *prhs, int *m, int *n);
wiByte     *WIAPI wiMexUtil_GetByteArray    (const mxArray *prhs, int *length);
wiUWORD    *WIAPI wiMexUtil_GetUWORDArray   (const mxArray *prhs, int *length);
int         WIAPI wiMexUtil_CopyIntArray    (const mxArray *prhs, int *x, int Nmax);
wiByte     *WIAPI wiMexUtil_GetBinaryArray  (const mxArray *prhs, int *length);

//-------- Put Values/Arrays into MATLAB ----------------------------------------------------------
void WIAPI wiMexUtil_PutIntValue      (int       x, mxArray **plhs);
void WIAPI wiMexUtil_PutUIntValue     (wiUInt    x, mxArray **plhs);
void WIAPI wiMexUtil_PutRealValue     (wiReal    x, mxArray **plhs);
void WIAPI wiMexUtil_PutBooleanValue  (wiBoolean x, mxArray **plhs);
void WIAPI wiMexUtil_PutString        (wiString  x, mxArray **plhs);
void WIAPI wiMexUtil_PutIntArray      (int n, int       *x, mxArray **plhs);
void WIAPI wiMexUtil_PutByteArray     (int n, unsigned char *x, mxArray **plhs);
void WIAPI wiMexUtil_PutRealArray     (int n, wiReal    *x, mxArray **plhs);
void WIAPI wiMexUtil_PutRealMatrix    (int m, int n, wiReal **x, mxArray **plhs);
void WIAPI wiMexUtil_PutComplexArray  (int n, wiComplex *x, mxArray **plhs);
void WIAPI wiMexUtil_PutComplexMatrix (int m, int n, wiComplex **x, mxArray **plhs);
void WIAPI wiMexUtil_PutByteArray     (int n, wiByte    *x, mxArray **plhs);
void WIAPI wiMexUtil_PutWORDArray     (int n, wiWORD    *x, mxArray **plhs);
void WIAPI wiMexUtil_PutUWORDArray    (int n, wiUWORD   *x, mxArray **plhs);
void WIAPI wiMexUtil_PutWORDPairArray (int n, wiWORD    *x, wiWORD *y, mxArray **plhs);
void WIAPI wiMexUtil_PutBinaryArray   (int n, void *x, mxArray **plhs);

//-------- Create mxArray With Data Type ----------------------------------------------------------
mxArray * WIAPI wiMexUtil_CreateUWORDArrayAsDouble (int n, wiUWORD *xReal, wiUWORD *xImag);
mxArray * WIAPI wiMexUtil_CreateWORDArrayAsDouble  (int n, wiWORD  *xReal, wiWORD  *xImag);
mxArray * WIAPI wiMexUtil_CreateComplexArray       (int n, wiComplex *x);
mxArray * WIAPI wiMexUtil_CreateUWORDMatrixAsDouble(int m, int n, wiUWORD *xReal[], wiUWORD *xImag[]);
mxArray * WIAPI wiMexUtil_CreateWORDMatrixAsDouble (int m, int n, wiWORD  *xReal[], wiWORD  *xImag[]);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

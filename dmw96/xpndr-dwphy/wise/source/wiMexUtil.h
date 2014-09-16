//
//  wiMexUtil.h -- Matlab Executable Utility Functions for WiSE
//

#ifndef __WIMEXUTIL_H
#define __WIMEXUTIL_H

#include "mex.h"    // include Matlab data types
#include "wiType.h" // include WiSE data types

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// ================================================================================================
// FUNCTION PROTOTYPES
// ================================================================================================
void       wiMexUtil_CheckParameters(int nlhs, int nrhs, int nlhs_req, int nrhs_req);
void       wiMexUtil_ResetMsgFunc(void);
wiString   wiMexUtil_GetMsgFuncBuffer(void);
wiBoolean  wiMexUtil_CommandMatch(char *cmd, char *TargetStr);

int __cdecl wiMexUtil_Output (const char *format, ...);
int __cdecl wiMexUtil_MsgFunc(const char *format, ...);

//-------- Get Values/Arrays from MATLAB ----------------------------------------------------------
wiString    wiMexUtil_GetStringValue  (const mxArray *prhs, int *length);
int         wiMexUtil_GetIntValue     (const mxArray *prhs);
wiUInt      wiMexUtil_GetUIntValue    (const mxArray *prhs);
wiReal      wiMexUtil_GetRealValue    (const mxArray *prhs);
wiBoolean   wiMexUtil_GetBooleanValue (const mxArray *prhs);
int        *wiMexUtil_GetIntArray     (const mxArray *prhs, int *length);
wiUInt     *wiMexUtil_GetUIntArray    (const mxArray *prhs, int *length);
wiReal     *wiMexUtil_GetRealArray    (const mxArray *prhs, int *length);
wiComplex  *wiMexUtil_GetComplexArray (const mxArray *prhs, int *length);
wiComplex **wiMexUtil_GetComplexMatrix(const mxArray *prhs, int *m, int *n);
wiByte     *wiMexUtil_GetByteArray    (const mxArray *prhs, int *length);
wiUWORD    *wiMexUtil_GetUWORDArray   (const mxArray *prhs, int *length);
int         wiMexUtil_CopyIntArray    (const mxArray *prhs, int *x, int Nmax);

//-------- Put Values/Arrays into MATLAB ----------------------------------------------------------
void wiMexUtil_PutIntValue      (int       x, mxArray **plhs);
void wiMexUtil_PutUIntValue     (wiUInt    x, mxArray **plhs);
void wiMexUtil_PutRealValue     (wiReal    x, mxArray **plhs);
void wiMexUtil_PutBooleanValue  (wiBoolean x, mxArray **plhs);
void wiMexUtil_PutString        (wiString  x, mxArray **plhs);
void wiMexUtil_PutIntArray      (int n, int           *x, mxArray **plhs);
void wiMexUtil_PutUIntArray     (int n, unsigned int  *x, mxArray **plhs);
void wiMexUtil_PutByteArray     (int n, unsigned char *x, mxArray **plhs);
void wiMexUtil_PutRealArray     (int n, wiReal        *x, mxArray **plhs);
void wiMexUtil_PutComplexArray  (int n, wiComplex     *x, mxArray **plhs);
void wiMexUtil_PutByteArray     (int n, wiByte        *x, mxArray **plhs);
void wiMexUtil_PutWORDArray     (int n, wiWORD        *x, mxArray **plhs);
void wiMexUtil_PutUWORDArray    (int n, wiUWORD       *x, mxArray **plhs);
void wiMexUtil_PutBinaryArray   (int n, void          *x, mxArray **plhs);
void wiMexUtil_PutWORDPairArray (int n, wiWORD  *x, wiWORD  *y, mxArray **plhs);
void wiMexUtil_PutUWORDPairArray(int n, wiUWORD *x, wiUWORD *y, mxArray **plhs);
void wiMexUtil_PutRealMatrix    (int m, int n, wiReal **x, mxArray **plhs);
void wiMexUtil_PutComplexMatrix (int m, int n, wiComplex **x, mxArray **plhs);

//-------- Create mxArray With Data Type ----------------------------------------------------------
mxArray * wiMexUtil_CreateUWORDArrayAsDouble (int n, wiUWORD *xReal, wiUWORD *xImag);
mxArray * wiMexUtil_CreateWORDArrayAsDouble  (int n, wiWORD  *xReal, wiWORD  *xImag);
mxArray * wiMexUtil_CreateRealArray          (int n, wiReal    *x);
mxArray * wiMexUtil_CreateComplexArray       (int n, wiComplex *x);
mxArray * wiMexUtil_CreateUWORDMatrixAsDouble(int m, int n, wiUWORD *xReal[], wiUWORD *xImag[]);
mxArray * wiMexUtil_CreateWORDMatrixAsDouble (int m, int n, wiWORD  *xReal[], wiWORD  *xImag[]);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

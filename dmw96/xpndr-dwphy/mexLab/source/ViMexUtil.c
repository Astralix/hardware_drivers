//--< ViMexUtil.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  VISA/MATLAB Executable Utilities
//
//  Written by Barrett Brickner
//  Copyright 2007 DSP Group. All Rights Reserved.
//
//  This module is derived from wiMexUtil in project WiSE
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#include <stdarg.h>
#include <string.h>
#include "ViMexUtil.h"

// ================================================================================================
// MODULE-SPECIFIC PARAMETERS
// ================================================================================================
static char *MsgFuncBuf = NULL;

// ================================================================================================
// FUNCTION   : ViMexUtil_CheckParameters()
// ------------------------------------------------------------------------------------------------
// Purpose    : Check the number of input and output parameters
// Parameters : nlhs     -- Number of actual left-hand-side parameters
//              nrhs     -- Number of actual right-hand-side parameters
//              nlhs_req -- Number of required lhs parameters
///             nrhs_req -- Number of required rhs parameters
// ================================================================================================
void ViMexUtil_CheckParameters(int nlhs, int nrhs, int nlhs_req, int nrhs_req)
{
    char msg[80];

    if(nrhs!=nrhs_req) 
    {
        sprintf(msg,"%d input parameters were expected, %d were received.\n",nrhs_req,nrhs);
        mexErrMsgTxt(msg);
    }
    if(nlhs!=nlhs_req) 
    {
        sprintf(msg,"%d output terms were expected, %d were received.\n",nlhs_req,nlhs);
        mexErrMsgTxt(msg);
    }
}
// end of MexUtil_CheckParameters()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetStringValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
ViString ViMexUtil_GetStringValue(const mxArray *prhs, int *length)
{
    int n = mxGetM(prhs) * mxGetN(prhs) + 1;
    char *buf = (char *)mxCalloc(n, sizeof(char));
    if(!buf) mexErrMsgTxt("Memory allocation error");

    if(mxGetString(prhs, buf, n))
        mexErrMsgTxt("A string argument could not be converted.");
    if(length) *length = n;

    return buf;
}
// end of ViMexUtil_GetStringValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetIntValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int ViMexUtil_GetIntValue(const mxArray *prhs)
{
    if((mxGetM(prhs)!=1)||(mxGetN(prhs)!=1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");

    return (int)mxGetScalar(prhs);
}
// end of ViMexUtil_GetIntValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetUIntValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
ViUInt32 ViMexUtil_GetUIntValue(const mxArray *prhs)
{
    if((mxGetM(prhs)!=1)||(mxGetN(prhs)!=1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");

    return (ViUInt32)mxGetScalar(prhs);
}
// end of ViMexUtil_GetUIntValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetRealValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
ViReal64 ViMexUtil_GetRealValue(const mxArray *prhs)
{
    if((mxGetM(prhs)!=1)||(mxGetN(prhs)!=1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");
    return (ViReal64)mxGetScalar(prhs);
}
// end of ViMexUtil_GetRealValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetBooleanValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
ViBoolean ViMexUtil_GetBooleanValue(const mxArray *prhs)
{
    if((mxGetM(prhs)!=1)||(mxGetN(prhs)!=1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");

    return (mxGetScalar(prhs)? VI_TRUE : VI_FALSE);
}
// end of ViMexUtil_GetBooleanValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns an integer array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
int * ViMexUtil_GetIntArray(const mxArray *prhs, int *length)
{
    int     i, m, n, N;
    double *dptr;
    int *x, *xptr;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    dptr = mxGetPr(prhs);
    x = xptr = (int *)mxCalloc(N,sizeof(int));
    for(i=0; i<N; i++, dptr++, xptr++)
        *xptr = (int)(*dptr);
    if(length) *length = N;
    return x;
}
// end of ViMexUtil_GetIntArray()

// ================================================================================================
// FUNCTION   : ViMexUtil_CopyIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Copies a Matlab array to a supplied integer array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              x      -- Pointer to the output array (pre-allocated)
//              Nmax   -- Maximum length of the output array
// Returns    : actual number of elements copied
// ================================================================================================
int ViMexUtil_CopyIntArray(const mxArray *prhs, int *x, int Nmax)
{
    int     i, m, n, N;
    double *dptr;
    int     *xptr = x;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n; 
    if(N>Nmax) N=Nmax;
    dptr = mxGetPr(prhs);
    for(i=0; i<N; i++, dptr++, xptr++)
        *xptr = (int)(*dptr);
    return N;
}
// end of ViMexUtil_CopyIntArray()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetBinaryArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a binary (ViUInt8) array from a Matlab uint8 numeric array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
ViUInt8* ViMexUtil_GetBinaryArray(const mxArray *prhs, int *length)
{
    int     m, n, N;
    ViUInt8 *x;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    x = (ViUInt8 *)mxCalloc(N,sizeof(ViUInt8));
    memcpy(x, mxGetData(prhs), N);
    if(length) *length = N;
    return x;
}
// end of ViMexUtil_GetBinaryArray()

// ================================================================================================
// FUNCTION   : ViMexUtil_GetRealArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a real array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
ViReal64 *  ViMexUtil_GetRealArray(const mxArray *prhs, int *length)
{
    int     i, m, n, N;
    double *dptr;
    ViReal64 *x, *xptr;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if((m!=1)&&(n!=1)) mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    dptr = mxGetPr(prhs);
    x = xptr = (ViReal64 *)mxCalloc(N,sizeof(ViReal64));
    for(i=0; i<N; i++, dptr++, xptr++)
        *xptr = (ViReal64)(*dptr);
    if(length) *length = N;
    return x;
}
// end of ViMexUtil_GetRealArray()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutIntValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert an integer value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutIntValue(int x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)x;
}
// end of ViMexUtil_PutIntValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutUIntValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a ViUInt32 value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutUIntValue(ViUInt32 x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)x;
}
// end of ViMexUtil_PutUIntValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutRealValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a ViReal64 value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutRealValue(ViReal64 x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)x;
}
// end of ViMexUtil_PutRealValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutBooleanValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a Boolean value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutBooleanValue(ViBoolean x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)(x? 1:0);
}
// end of ViMexUtil_PutBooleanValue()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutString()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a string array to a Matlab array
// Parameters : x    -- Pointer to the null-terminated string (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutString(ViString x, mxArray **plhs)
{
    *plhs = mxCreateString(x);
}
// end of ViMexUtil_PutString()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert an integer vector to a Matlab array
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutIntArray(int n, int *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR  = mxGetPr(*plhs);
    for(i=0; i<n; i++, pR++, x++)
        *pR = (double)(*x);
}
// end of ViMexUtil_PutIntArray()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutRealArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a ViReal64 vector to a Matlab array
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutRealArray(int n, ViReal64 *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR = mxGetPr(*plhs);
    for(i=0; i<n; i++, pR++, x++)
        *pR = (double)(*x);
}
// end of ViMexUtil_PutRealArray()

// ================================================================================================
// FUNCTION   : ViMexUtil_PutBinaryArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a binary array to a MEX argument as type uint8
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void  ViMexUtil_PutBinaryArray(int n, void *x, mxArray **plhs)
{
    void *pData;

    *plhs = mxCreateNumericArray(1, (const int *)(&n), mxUINT8_CLASS, mxREAL);
    pData = mxGetData(*plhs);
    memcpy(pData, x, n);
}
// end of ViMexUtil_PutBinaryArray()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

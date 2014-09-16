//--< wiMexUtil.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Matlab Executable Utilities
//
//  This file contains general purpose utility functions for converting between 'c' and Matlab
//  data types.
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#include <stdarg.h>
#include <string.h>
#include "wiMexUtil.h"

// ================================================================================================
// MODULE-SPECIFIC PARAMETERS
// ================================================================================================
static char *MsgFuncBuf = NULL;

// ================================================================================================
// FUNCTION   : wiMexUtil_CheckParameters()
// ------------------------------------------------------------------------------------------------
// Purpose    : Check the number of input and output parameters
// Parameters : nlhs     -- Number of actual left-hand-side parameters
//              nrhs     -- Number of actual right-hand-side parameters
//              nlhs_req -- Number of required lhs parameters
///             nrhs_req -- Number of required rhs parameters
// ================================================================================================
void wiMexUtil_CheckParameters(int nlhs, int nrhs, int nlhs_req, int nrhs_req)
{
    char msg[80];

    if (nrhs!=nrhs_req) {
        sprintf(msg,"%d input parameters were expected, %d were received.\n",nrhs_req,nrhs);
        mexErrMsgTxt(msg);
    }
    if (nlhs!=nlhs_req) {
        sprintf(msg,"%d output terms were expected, %d were received.\n",nlhs_req,nlhs);
        mexErrMsgTxt(msg);
    }
}
// end of MexUtil_CheckParameters()

// ================================================================================================
// FUNCTION   : wiMexUtil_Output()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write output via mexPrintf
// Parameters : sames as printf
// ================================================================================================
int __cdecl wiMexUtil_Output(const char *format, ...)
{
    va_list ap;
    char buf[1024];

    va_start(ap,format);
    vsprintf(buf,format,ap);
    va_end(ap);
    mexPrintf(buf);

    return 0;
}
// end of wiMexUtil_Output()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetStringValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiString wiMexUtil_GetStringValue(const mxArray *prhs, int *length)
{
    int n = mxGetM(prhs) * mxGetN(prhs) + 1;
    char *buf = (char *)mxCalloc(n, sizeof(char));
    if (!buf) mexErrMsgTxt("Memory allocation error");

    if (mxGetString(prhs, buf, n))
        mexErrMsgTxt("A string argument could not be converted.");
    if (length) *length = n;
    return buf;
}
// end of wiMexUtil_GetStringValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetIntValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int wiMexUtil_GetIntValue(const mxArray *prhs)
{
    if ((mxGetM(prhs) != 1) || (mxGetN(prhs) != 1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");
    return (int)mxGetScalar(prhs);
}
// end of wiMexUtil_GetIntValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetUIntValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiUInt wiMexUtil_GetUIntValue(const mxArray *prhs)
{
    if ((mxGetM(prhs) !=1 ) || (mxGetN(prhs) != 1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");
    return (wiUInt)mxGetScalar(prhs);
}
// end of wiMexUtil_GetUIntValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetRealValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal wiMexUtil_GetRealValue(const mxArray *prhs)
{
    if ((mxGetM(prhs) != 1) || (mxGetN(prhs) != 1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");
    return (wiReal)mxGetScalar(prhs);
}
// end of wiMexUtil_GetRealValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetBooleanValue()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiBoolean wiMexUtil_GetBooleanValue(const mxArray *prhs)
{
    if ((mxGetM(prhs) != 1)||(mxGetN(prhs) != 1))
        mexErrMsgTxt("A vector/matrix was passed when a scalar was expected.");
    return (mxGetScalar(prhs)? WI_TRUE : WI_FALSE);
}
// end of wiMexUtil_GetBooleanValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns an integer array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
int * wiMexUtil_GetIntArray(const mxArray *prhs, int *length)
{
    int     i, m, n, N;
    double *dptr;
    int *x, *xptr;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if ((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    dptr = mxGetPr(prhs);
    x = xptr = (int *)mxCalloc(N,sizeof(int));
    for (i=0; i<N; i++, dptr++, xptr++)
        *xptr = (int)(*dptr);
    if (length) *length = N;
    return x;
}
// end of wiMexUtil_GetIntArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetUIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns an unsigned integer array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
wiUInt * wiMexUtil_GetUIntArray(const mxArray *prhs, int *length)
{
    int     i, m, n, N;
    double *dptr;
    wiUInt *x, *xptr;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if ((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    dptr = mxGetPr(prhs);
    x = xptr = (wiUInt *)mxCalloc(N,sizeof(wiUInt));
    for (i=0; i<N; i++, dptr++, xptr++)
        *xptr = (wiUInt)(*dptr);
    if (length) *length = N;
    return x;
}
// end of wiMexUtil_GetUIntArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_CopyIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Copies a Matlab array to a supplied integer array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              x      -- Pointer to the output array (pre-allocated)
//              Nmax   -- Maximum length of the output array
// Returns    : actual number of elements copied
// ================================================================================================
int wiMexUtil_CopyIntArray(const mxArray *prhs, int *x, int Nmax)
{
    int     i, m, n, N;
    double *dptr;
    int     *xptr = x;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if ((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n; 
    if (N>Nmax) N=Nmax;
    dptr = mxGetPr(prhs);
    for (i=0; i<N; i++, dptr++, xptr++)
        *xptr = (int)(*dptr);
    return N;
}
// end of wiMexUtil_CopyIntArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetByteArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a wiByte array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
wiByte * wiMexUtil_GetByteArray(const mxArray *prhs, int *length)
{
    int     i, m, n, N;
    double *dptr;
    wiByte *x, *xptr;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if ((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    dptr = mxGetPr(prhs);
    x = xptr = (wiByte *)mxCalloc(N,sizeof(wiByte));
    for (i=0; i<N; i++, dptr++, xptr++)
        *xptr = (wiByte)(*dptr);
    if (length) *length = N;
    return x;
}
// end of wiMexUtil_GetByteArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetBinaryArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a binary (wiByte) array from a Matlab uint8 numeric array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
wiByte * wiMexUtil_GetBinaryArray(const mxArray *prhs, int *length)
{
    int     m, n, N;
    wiByte *x;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if ((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    x = (wiByte *)mxCalloc(N,sizeof(wiByte));
    memcpy(x, mxGetData(prhs), N);
    if (length) *length = N;
    return x;
}
// end of wiMexUtil_GetBinaryArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetUWORDArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a wiUWORD array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
wiUWORD * wiMexUtil_GetUWORDArray(const mxArray *prhs, int *length)
{
    int     i, m, n, N;
    double *dptr;
    wiUWORD *x, *xptr;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if ((m!=1) && (n!=1))
        mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    dptr = mxGetPr(prhs);
    x = xptr = (wiUWORD *)mxCalloc(N,sizeof(wiUWORD));
    for (i=0; i<N; i++, dptr++, xptr++)
        xptr->word = (unsigned)(*dptr);
    if (length) *length = N;
    return x;
}
// end of wiMexUtil_GetUWORDArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetRealArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a real array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
wiReal * wiMexUtil_GetRealArray(const mxArray *prhs, int *length)
{
    int     i, m, n, N;
    double *dptr;
    wiReal *x, *xptr;

    m = mxGetM(prhs);
    n = mxGetN(prhs);
    if ((m!=1) && (n!=1)) mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    dptr = mxGetPr(prhs);
    x = xptr = (wiReal *)mxCalloc(N,sizeof(wiReal));
    for (i=0; i<N; i++, dptr++, xptr++)
        *xptr = (wiReal)(*dptr);
    if (length) *length = N;
    return x;
}
// end of wiMexUtil_GetRealArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetComplexArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a complex array from a Matlab array
// Parameters : prhs   -- Pointer to a matlab array (input)
//              length -- Pointer to the array length (output)
// ================================================================================================
wiComplex * wiMexUtil_GetComplexArray(const mxArray *prhs, int *length)
{
    int        i, m, n, N;
    double    *pR, *pI;
    wiComplex *x;

    m = mxGetM(prhs); n = mxGetN(prhs);
    if ((m!=1) && (n!=1)) mexErrMsgTxt("A matrix was received where a vector was expected.");
    N = m*n;
    pR = mxGetPr(prhs);
    pI = mxGetPi(prhs);
    x = (wiComplex *)mxCalloc(N, sizeof(wiComplex));

    if (pI)
    {
        for (i=0; i<N; i++) {
            x[i].Real = (wiReal)pR[i];
            x[i].Imag = (wiReal)pI[i];
        }
    }
    else
    {
        for (i=0; i<N; i++) {
            x[i].Real = (wiReal)pR[i];
            x[i].Imag = 0.0;
        }
    }
    if (length) *length = N;
    return x;
}
// end of wiMexUtil_GetComplexArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetComplexMatrix()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a complex matrix from a Matlab array
// Parameters : prhs -- Pointer to a matlab array (input)
//              pm   -- Pointer to the number of columns (output)
//              pn   -- Pointer to the number of rows (output)
// ================================================================================================
wiComplex ** wiMexUtil_GetComplexMatrix(const mxArray *prhs, int *pm, int *pn)
{
    int        i, j, k=0, m, n;
    double    *pR, *pI;
    wiComplex **X;

    m = mxGetM(prhs); n = mxGetN(prhs);
    pR = mxGetPr(prhs);
    pI = mxGetPi(prhs);

    X = (wiComplex **)mxCalloc(n+1, sizeof(wiComplex));
    for (j=0; j<n; j++)
        X[j] = (wiComplex *)mxCalloc(m, sizeof(wiComplex));
    X[n] = NULL; // terminator

    if (pI)
    {
        for (j=0; j<n; j++) {
            for (i=0; i<m; i++)
            {
                X[j][i].Real = (wiReal)pR[k];
                X[j][i].Imag = (wiReal)pI[k];
                k++;
            }
        }
    }
    else
    {
        for (j=0; j<n; j++) {
            for (i=0; i<m; i++)
            {
                X[j][i].Real = (wiReal)pR[k];
                X[j][i].Imag = 0.0;
                k++;
            }
        }
    }
    if (pm) *pm = m;
    if (pn) *pn = n;
    return X;
}
// end of wiMexUtil_GetComplexMatrix()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutIntValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert an integer value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutIntValue(wiInt x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)x;
}
// end of wiMexUtil_PutIntValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutUIntValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a wiUInt value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutUIntValue(wiUInt x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)x;
}
// end of wiMexUtil_PutUIntValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutRealValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a wiReal value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutRealValue(wiReal x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)x;
}
// end of wiMexUtil_PutRealValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutBooleanValue()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a Boolean value to a Matlab array
// Parameters : x    -- Value to be converted
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutBooleanValue(wiBoolean x, mxArray **plhs)
{
    *plhs = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(*plhs) = (double)(x? 1:0);
}
// end of wiMexUtil_PutBooleanValue()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutString()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a string array to a Matlab array
// Parameters : x    -- Pointer to the null-terminated string (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutString(wiString x, mxArray **plhs)
{
    *plhs = mxCreateString(x);
}
// end of wiMexUtil_PutString()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert an integer vector to a Matlab array
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutIntArray(int n, int *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR  = mxGetPr(*plhs);
    for (i=0; i<n; i++, pR++, x++)
        *pR = (double)(*x);
}
// end of wiMexUtil_PutIntArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutUIntArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert an unsigned integer vector to a Matlab array
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutUIntArray(int n, unsigned int *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR  = mxGetPr(*plhs);
    for (i=0; i<n; i++, pR++, x++)
        *pR = (double)(*x);
}
// end of wiMexUtil_PutIntArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutRealArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a wiReal vector to a Matlab array
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutRealArray(int n, wiReal *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR = mxGetPr(*plhs);
    for (i=0; i<n; i++, pR++, x++)
        *pR = (double)(*x);
}
// end of wiMexUtil_PutRealArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutComplexArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a wiComplex vector to a Matlab array
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutComplexArray(int n, wiComplex *x, mxArray **plhs)
{
    int     i;
    double *pR, *pI;

    *plhs = mxCreateDoubleMatrix(n, 1, mxCOMPLEX);
    pR  = mxGetPr(*plhs);  pI  = mxGetPi(*plhs);
    for (i=0; i<n; i++, x++, pR++, pI++)
    {
        *pR = (double)(x->Real);
        *pI = (double)(x->Imag);
    }
}
// end of wiMexUtil_PutComplexArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutComplexMatrix()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a complex matrix to a Matlab array
// Parameters : m    -- Number of columns
//              n    -- Number of rows
//              x    -- Pointer to the 2-dimensional complex array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutComplexMatrix(int m, int n, wiComplex **x, mxArray **plhs)
{
    int     i, j, k=0;
    double *pR, *pI;

    *plhs = mxCreateDoubleMatrix(n, m, mxCOMPLEX);
    pR  = mxGetPr(*plhs);  pI  = mxGetPi(*plhs);

    for (j=0; j<m; j++)
    {
        for (i=0; i<n; i++)
        {
            pR[k] = (double)(x[j][i].Real);
            pI[k] = (double)(x[j][i].Imag);
            k++;
        }
    }
}
// end of wiMexUtil_PutComplexMatrix()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutRealMatrix()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a real matrix to a Matlab array
// Parameters : m    -- Number of columns
//              n    -- Number of rows
//              x    -- Pointer to the 2-dimensional array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutRealMatrix(int m, int n, wiReal **x, mxArray **plhs)
{
    int     i, j, k=0;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, m, mxREAL);
    pR  = mxGetPr(*plhs);
    for (j=0; j<m; j++) {
        for (i=0; i<n; i++) {
            pR[k] = (double)(x[j][i]);
            k++;
        }
    }
}
// end of wiMexUtil_PutRealMatrix()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutBinaryArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a binary array to a MEX argument as type uint8
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutBinaryArray(int n, void *x, mxArray **plhs)
{
    void *pData;
    mwSize Size = n;

    *plhs = mxCreateNumericArray(1, &Size, mxUINT8_CLASS, mxREAL);
    pData = mxGetData(*plhs);
    memcpy(pData, x, n);
}
// end of wiMexUtil_PutBinaryArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutByteArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a wiByte vector to a MEX argument
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutByteArray(int n, wiByte *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR  = mxGetPr(*plhs);
    for (i=0; i<n; i++, pR++, x++)
    {
        *pR = *x;
    }
}
// end of wiMexUtil_PutByteArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutWORDArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a wiUWORD vector to a MEX argument
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutWORDArray(int n, wiWORD *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR  = mxGetPr(*plhs);
    for (i=0; i<n; i++, pR++, x++)
        *pR = (double)x->word;
}
// end of wiMexUtil_PutWORDArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutUWORDArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a wiUWORD vector to a MEX argument
// Parameters : n    -- Length of the array
//              x    -- Pointer to the array (input)
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutUWORDArray(int n, wiUWORD *x, mxArray **plhs)
{
    int     i;
    double *pR;

    *plhs = mxCreateDoubleMatrix(n, 1, mxREAL);
    pR  = mxGetPr(*plhs);
    for (i=0; i<n; i++, pR++, x++)
        *pR = (double)x->word;
}
// end of wiMexUtil_PutUWORDArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutWORDPairArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a pair of wiWORD vectors representing real and imaginary complex components
//              to a MEX argument
// Parameters : n    -- Length of the arrays
//              x    -- Pointer to the real component array
//              y    -- Pointer to the imag component array
//              plhs -- Pointer to matlab array (output)
// ================================================================================================
void wiMexUtil_PutWORDPairArray(int n, wiWORD *x, wiWORD *y, mxArray **plhs)
{
    int     i;
    double *pR, *pI;

    *plhs = mxCreateDoubleMatrix(n, 1, mxCOMPLEX);
    pR  = mxGetPr(*plhs);
    pI  = mxGetPi(*plhs);
    for (i=0; i<n; i++, pR++, pI++, x++, y++)
    {
        *pR = (double)x->word;
        *pI = (double)y->word;
    }
}
// end of wiMexUtil_PutWORDArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_PutUWORDPairArray()
// ------------------------------------------------------------------------------------------------
// Purpose    : Convert a pair of wiUWORD vectors representing real and imaginary complex
//              components to a MEX argument
// ================================================================================================
void wiMexUtil_PutUWORDPairArray(int n, wiUWORD *x, wiUWORD *y, mxArray **plhs)
{
    int     i;
    double *pR, *pI;

    *plhs = mxCreateDoubleMatrix(n, 1, mxCOMPLEX);
    pR  = mxGetPr(*plhs);
    pI  = mxGetPi(*plhs);
    for (i=0; i<n; i++, pR++, pI++, x++, y++)
    {
        *pR = (double)x->word;
        *pI = (double)y->word;
    }
}
// end of wiMexUtil_PutUWORDArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_ResetMsgFunc()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiMexUtil_ResetMsgFunc(void)
{
    if (MsgFuncBuf) mxFree(MsgFuncBuf);
    MsgFuncBuf = NULL;
}
// end of wiMexUtil_ResetMsgFunc()

// ================================================================================================
// FUNCTION   : wiMexUtil_MsgFunc()
// ------------------------------------------------------------------------------------------------
// Purpose    : Buffer the input to this MsgFunc for later retrieval
// Parameters : same as printf()
// ================================================================================================
int __cdecl wiMexUtil_MsgFunc(const char *format, ...)
{
    char tmpbuf[1024], *newbuf;
    int   n;
    va_list ap;

    va_start(ap, format);
    vsprintf(tmpbuf, format, ap);
    va_end(ap);

    n = strlen(tmpbuf);
    if (MsgFuncBuf != NULL) n += strlen(MsgFuncBuf);
    newbuf = (char *)mxRealloc(MsgFuncBuf, n+1);
    if (!newbuf) mexErrMsgTxt("Memory allocation fault in wiMexUtil_MsgFunc()");
    if (MsgFuncBuf == NULL) *newbuf = 0;

    strcat(newbuf, tmpbuf);
    MsgFuncBuf = newbuf;

    return 0;
}
// end of wiMexUtil_MsgFunc()

// ================================================================================================
// FUNCTION   : wiMexUtil_GetMsgFuncBuffer()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiString wiMexUtil_GetMsgFuncBuffer(void)
{
    return MsgFuncBuf;
}
// end of wiMexUtil_GetMsgFuncBuffer()

// ================================================================================================
// FUNCTION   : wiMexUtil_CommandMatch()
// ------------------------------------------------------------------------------------------------
// Purpose    : Look for a matching command where the input is compared against both the TargetStr
//              and that same value with "()" appended.
// ================================================================================================
wiBoolean wiMexUtil_CommandMatch(char *cmd, char *TargetStr)
{
    if (strcmp(cmd, TargetStr) == 0) return 1; // identical

    if (strstr(cmd, TargetStr) == cmd) // cmd begins with TargetStr
    {
        // return 1 if the string following TargetStr is exactly "()"; otherwise return 0
        //
        return (strcmp(cmd + strlen(TargetStr), "()") == 0) ? 1:0;
    }
    else return 0;
}
// end of wiMexUtil_CommandMatch()

// ================================================================================================
// FUNCTION   : wiMexUtil_CreateUWORDArrayAsDouble()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
mxArray * wiMexUtil_CreateUWORDArrayAsDouble(int n, wiUWORD *xReal, wiUWORD *xImag)
{
    mxArray *A; double *pR, *pI; int i;
    
    if (xImag)
    {
        A = mxCreateDoubleMatrix(n,1, mxCOMPLEX);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);  pI = mxGetPi(A);
        for (i=0; i<n; i++, pR++, pI++, xReal++, xImag++) {
            *pR = (double)(xReal->word);
            *pI = (double)(xImag->word);
        }
    }
    else
    {
        A = mxCreateDoubleMatrix(n,1, xImag? mxCOMPLEX:mxREAL);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);
        for (i=0; i<n; i++, pR++, xReal++)
            *pR = (double)(xReal->word);
    }
    return A;
}
// end of wiMexUtil_CreateUWORDArrayAsDouble()

// ================================================================================================
// FUNCTION   : wiMexUtil_CreateWORDArrayAsDouble()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
mxArray * wiMexUtil_CreateWORDArrayAsDouble(int n, wiWORD *xReal, wiWORD *xImag)
{
    mxArray *A; double *pR, *pI; int i;
    
    if (xImag)
    {
        A = mxCreateDoubleMatrix(n,1, mxCOMPLEX);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);  pI = mxGetPi(A);
        for (i=0; i<n; i++, pR++, pI++, xReal++, xImag++) {
            *pR = (double)(xReal->word);
            *pI = (double)(xImag->word);
        }
    }
    else
    {
        A = mxCreateDoubleMatrix(n,1, xImag? mxCOMPLEX:mxREAL);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);
        for (i=0; i<n; i++, pR++, xReal++)
            *pR = (double)(xReal->word);
    }
    return A;
}
// end of wiMexUtil_CreateWORDArrayAsDouble()

// ================================================================================================
// FUNCTION   : wiMexUtil_CreateComplexArray()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
mxArray * wiMexUtil_CreateComplexArray(int n, wiComplex *x)
{
    mxArray *A; double *pR, *pI; int i;
    
    A = mxCreateDoubleMatrix(n,1, mxCOMPLEX);
    if (!A) mexErrMsgTxt("Memory Allocation Error");
    pR = mxGetPr(A);
    pI = mxGetPi(A);
    for (i=0; i<n; i++, pR++, pI++, x++) {
        *pR = x->Real;
        *pI = x->Imag;
    }
    return A;
}
// end of wiMexUtil_CreateComplexArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_CreateRealArray()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
mxArray * wiMexUtil_CreateRealArray(int n, wiReal *x)
{
    mxArray *A; double *pR; int i;
    
    A = mxCreateDoubleMatrix(n,1, mxREAL);
    if (!A) mexErrMsgTxt("Memory Allocation Error");
    pR = mxGetPr(A);
    for (i=0; i<n; i++, pR++, x++) {
        *pR = *x;
    }
    return A;
}
// end of wiMexUtil_CreateRealArray()

// ================================================================================================
// FUNCTION   : wiMexUtil_CreateUWORDMatrixAsDouble()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
mxArray * wiMexUtil_CreateUWORDMatrixAsDouble(int m, int n, wiUWORD **xReal, wiUWORD **xImag)
{
    mxArray *A; double *pR, *pI; 
    int i, j;
    
    if (xImag)
    {
        A = mxCreateDoubleMatrix(n,m, mxCOMPLEX);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);  pI = mxGetPi(A);
        for (j=0; j<m; j++) {
            for (i=0; i<n; i++, pR++, pI++) {
                *pR = (double)(xReal[j][i].word);
                *pI = (double)(xImag[j][i].word);
            }
        }
    }
    else
    {
        A = mxCreateDoubleMatrix(n,m, xImag? mxCOMPLEX:mxREAL);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);
        for (j=0; j<m; j++) {
            for (i=0; i<n; i++, pR++)
                *pR = (double)(xReal[j][i].word);
        }
    }
    return A;
}
// end of wiMexUtil_CreateUWORDMatrixAsDouble()

// ================================================================================================
// FUNCTION   : wiMexUtil_CreateWORDMatrixAsDouble()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
mxArray * wiMexUtil_CreateWORDMatrixAsDouble(int m, int n, wiWORD **xReal, wiWORD **xImag)
{
    mxArray *A; double *pR, *pI; 
    int i, j;
    
    if (xImag)
    {
        A = mxCreateDoubleMatrix(n,m, mxCOMPLEX);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);  pI = mxGetPi(A);
        for (j=0; j<m; j++) {
            for (i=0; i<n; i++, pR++, pI++) {
                *pR = (double)(xReal[j][i].word);
                *pI = (double)(xImag[j][i].word);
            }
        }
    }
    else
    {
        A = mxCreateDoubleMatrix(n,m, xImag? mxCOMPLEX:mxREAL);
        if (!A) mexErrMsgTxt("Memory Allocation Error");
        pR = mxGetPr(A);
        for (j=0; j<m; j++) {
            for (i=0; i<n; i++, pR++)
                *pR = (double)(xReal[j][i].word);
        }
    }
    return A;
}
// end of wiMexUtil_CreateWORDMatrixAsDouble()


//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

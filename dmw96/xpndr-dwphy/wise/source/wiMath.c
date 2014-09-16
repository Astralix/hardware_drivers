//--< wiMath.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Basic Mathematics Function Library
//  Copyright 2001 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "wiMath.h"
#include "wiUtil.h"

// Global variables
//
const wiComplex wiComplexZero = {0, 0};
const wiComplex wiComplexOne  = {1, 0};

// Local variables
//
static const double pi = 3.14159265358979323846;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(Status)  WICHK((Status))
#define XSTATUS(Status) if (STATUS(Status)<0) return WI_ERROR_RETURNED;

// ================================================================================================
// FUNCTION  : wiComplexNeg()
// ------------------------------------------------------------------------------------------------
// Purpose   : y = -x
// ================================================================================================
wiComplex wiComplexNeg(wiComplex x)
{
    wiComplex y;
    y.Real = -x.Real;
    y.Imag = -x.Imag;
    return y;
}
// end of wiComplexNeg()

// ================================================================================================
// FUNCTION  : wiComplexConj()
// ------------------------------------------------------------------------------------------------
// Purpose   : y = x*
// ================================================================================================
wiComplex wiComplexConj(wiComplex x)
{
    wiComplex y;
    y.Real =  x.Real;
    y.Imag = -x.Imag;
    return y;
}
// end of wiComplexConj()

// ================================================================================================
// FUNCTION  : wiComplexNorm()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return the L2 norm of the complex variable x
// ================================================================================================
wiReal wiComplexNorm(wiComplex x)
{
    return sqrt(x.Real*x.Real + x.Imag*x.Imag);
}
// end of wiComplexNorm()

// ================================================================================================
// FUNCTION  : wiComplexNormSquared()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return the squared L2 norm of the complex variable x
// ================================================================================================
wiReal wiComplexNormSquared(wiComplex x)
{
    return (x.Real*x.Real + x.Imag*x.Imag);
}
// end of wiComplexNormSquared()

// ================================================================================================
// FUNCTION  : wiComplexMul()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute c = a*b using complex variables
// ================================================================================================
wiComplex wiComplexMul(wiComplex a, wiComplex b)
{
    wiComplex c;
    c.Real = (a.Real * b.Real) - (a.Imag * b.Imag);                      
    c.Imag = (a.Real * b.Imag) + (a.Imag * b.Real);     

    return c;
}
// end of wiComplexMul()

// ================================================================================================
// FUNCTION  : wiComplexDiv()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute c = a/b using complex values
// ================================================================================================
wiComplex wiComplexDiv(wiComplex a, wiComplex b)
{
    wiComplex bInv = wiComplexInv(b);
    wiComplex c    = wiComplexMul(a, bInv);

    return c;
}
// end of wiComplexDiv()

// ================================================================================================
// FUNCTION  : wiComplexInv()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute c = 1/b using complex values
// ================================================================================================
wiComplex wiComplexInv(wiComplex b)
{
    wiComplex c;

    wiReal d2 = (b.Real * b.Real) + (b.Imag * b.Imag);

    c.Real =  b.Real/d2;
    c.Imag = -b.Imag/d2;

    return c;
}
// end of wiComplexInv()

// ================================================================================================
// FUNCTION  : wiComplexAdd()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute c = a+b using complex values
// ================================================================================================
wiComplex wiComplexAdd(wiComplex a, wiComplex b)
{
    wiComplex c;

    c.Real = a.Real + b.Real;
    c.Imag = a.Imag + b.Imag;

    return c;
}
// end of wiComplexSub()

// ================================================================================================
// FUNCTION  : wiComplexSub()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute c = a-b using complex values
// ================================================================================================
wiComplex wiComplexSub(wiComplex a, wiComplex b)
{
    wiComplex c;

    c.Real = a.Real - b.Real;
    c.Imag = a.Imag - b.Imag;

    return c;
}
// end of wiComplexSub()

// ================================================================================================
// FUNCTION  : wiComplexScaled()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute c = ab where a is complex is b is real
// ================================================================================================
wiComplex wiComplexScaled(wiComplex a, wiReal b)
{
    wiComplex c;

    c.Real = b * a.Real;
    c.Imag = b * a.Imag;

    return c;
}
// end of wiComplexScaled()

// ================================================================================================
// FUNCTION  : wiComplexExp()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute c = exp(ja)
// ================================================================================================
wiComplex wiComplexExp(wiReal a)
{
    wiComplex c;

    c.Real = cos(a);
    c.Imag = sin(a);

    return c;
}
// end of wiComplexExp()

// ================================================================================================
// FUNCTION  : wiComplexReal()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal wiComplexReal(wiComplex a)
{
    return a.Real;
}
// end of wiComplexReal()

// ================================================================================================
// FUNCTION  : wiComplexImag()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal wiComplexImag(wiComplex a)
{
    return a.Imag;
}
// end of wiComplexImag()

// ================================================================================================
// FUNCTION  : wiMath_BinomialExpansion()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute P(x) = (b[0] + b[1]x)^n
// ================================================================================================
wiStatus wiMath_BinomialExpansion(int n, double *b, double *p)
{
    int k, c;

    for (k = 0; k <= n; k++)
    {
        wiMath_Combinations(n, k, &c);
        p[k] = pow(b[0], (double)(n-k)) * pow(b[1], (double)k) * c;
    }
    return WI_SUCCESS;
}
// end of wiMath_BinomialExpansion()

// ================================================================================================
// FUNCTION  : wiMath_Combinations()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute number of combinations of k items from n.
// Parameters: n, k
// ================================================================================================
wiStatus wiMath_Combinations(int n, int k, int *c)
{
    int i;
    long num=1, den=1;
    
    for (i = 2; i <= n; i++)
        num *= i;

    for (i = 2; i <= k; i++)
        den *= i;
    for (i = 1; i <= (n-k); i++)
        den *= i;

    *c = num / den;

    return WI_SUCCESS;
}
// end of wiMath_Combinations()

// ================================================================================================
// FUNCTION  : wiMath_ComplexConvolve_Accumulate()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute y = y + conv(x,h) for complex values
// Parameters: Ny, y -- length of y and output array
//             Nx, x -- length of x and input array #1
//             Nh, h -- length of h and input array #2
// ================================================================================================
wiStatus wiMath_ComplexConvolve_Accumulate(int Ny, wiComplex y[], int Nx, wiComplex x[], int Nh, wiComplex h[])
{
    int i, k;
    int N = (Nx < Ny) ? Nx:Ny;

    // -----------
    // Convolution
    // -----------
    for (k = 0; k < Nh; k++)
    {
        for (i = 0; i <= k; i++)
        {
            y[k].Real += (x[k-i].Real * h[i].Real);
            y[k].Real -= (x[k-i].Imag * h[i].Imag);

            y[k].Imag += (x[k-i].Imag * h[i].Real);
            y[k].Imag += (x[k-i].Real * h[i].Imag);
        }
    }
    for ( ; k < N; k++)
    {
       for (i = 0; i < Nh; i++)
        {
            y[k].Real += (x[k-i].Real * h[i].Real);
            y[k].Real -= (x[k-i].Imag * h[i].Imag);

            y[k].Imag += (x[k-i].Imag * h[i].Real);
            y[k].Imag += (x[k-i].Real * h[i].Imag);
        }
    }
    for ( ; k < Ny; k++)
    {
       for (i = 0; i < Nh; i++)
        {
            if (k-i >= Nx) continue;
            y[k].Real += (x[k-i].Real * h[i].Real);
            y[k].Real -= (x[k-i].Imag * h[i].Imag);

            y[k].Imag += (x[k-i].Imag * h[i].Real);
            y[k].Imag += (x[k-i].Real * h[i].Imag);
        }
    }
    return WI_SUCCESS;
}
// end of wiMath_ComplexConvolve_Accumulate()

// ================================================================================================
// FUNCTION  : wiMath_ComplexConvolve()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute y = conv(x,h) for complex values
// Parameters: Ny, y -- length of y and output array
//             Nx, x -- length of x and input array #1
//             Nh, h -- length of h and input array #2
// ================================================================================================
wiStatus wiMath_ComplexConvolve(int Ny, wiComplex y[], int Nx, wiComplex x[], int Nh, wiComplex h[])
{
    memset(y, 0, Ny * sizeof(wiComplex));                            // clear the output array
    XSTATUS(wiMath_ComplexConvolve_Accumulate(Ny, y, Nx, x, Nh, h)); // compute y = y + conv(x,h)

    return WI_SUCCESS;
}
// end of wiMath_ComplexConvolve()

// ================================================================================================
// FUNCTION  : wiMath_FletcherChecksum()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute Fletcher's checksum on an array of bytes
// Reference : http://en.wikipedia.org/wiki/Fletcher%27s_checksum
// Parameters: NumBytes  -- length of array "x" in bytes
//             ptr       -- array cast as wiByte with data (if NULL, return initial value)
//             pZ        -- output value (also used as initial value if present)
// ================================================================================================
wiUInt wiMath_FletcherChecksum(wiUInt NumBytes, void *ptr, wiUInt *pZ)
{
    wiByte *x = (wiByte *)ptr;

    wiUInt sum1 = 0xFFFF, sum2 = 0xFFFF;
    wiUInt Y;

    NumBytes = NumBytes / 2; // number of 16-bit words computed from number of bytes

    if (pZ)
    {
        sum1 = (*pZ    ) & 0xFFFF;
        sum2 = (*pZ>>16) & 0xFFFF;
    }
    if (x)
    {
        while (NumBytes) 
        {
            unsigned int n = (NumBytes > 360) ? 360 : NumBytes;
            NumBytes -= n;
            do {
                sum1 += *(x++) << 8;
                sum1 += *(x++);
                sum2 += sum1;
            } while (--n);
            sum1 = (sum1 & 0xFFFF) + (sum1 >> 16);
            sum2 = (sum2 & 0xFFFF) + (sum2 >> 16);
        }
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    }
    Y = sum2 << 16 | sum1;

    if (pZ) *pZ = Y;
    return Y;
}
// end of wiMath_FletcherChecksum()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

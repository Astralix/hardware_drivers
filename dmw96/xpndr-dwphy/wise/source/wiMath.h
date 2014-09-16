//
//  wiMath.h -- WISE Mathematics Library
//  Copyright 2001 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.
//

#ifndef __WIMATH_H
#define __WIMATH_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// Long-name aliases for complex operation functions
//
#define wiComplexSubtract           wiComplexSub
#define wiComplexMultiply           wiComplexMul
#define wiComplexDivide             wiComplexDiv
#define wiComplexInverse            wiComplexInv
#define wiCompelxConjugate          wiComplexConj
#define wiComplexNormSquared        wiComplexNormSqr

// Global complex constants
extern const wiComplex wiComplexZero;
extern const wiComplex wiComplexOne;

//=================================================================================================
//  FUNCTION PROTOTYPES
//=================================================================================================
//
//  Complex Functions 
//
wiReal    wiComplexNorm   (wiComplex x);                 // compute the norm : ||x||
wiReal    wiComplexNormSqr(wiComplex x);                 // squared-norm     : ||x||^2
wiComplex wiComplexAdd    (wiComplex a, wiComplex b);    // complex addition : a+b
wiComplex wiComplexSub    (wiComplex a, wiComplex b);    // complex subtract : a-b
wiComplex wiComplexMul    (wiComplex a, wiComplex b);    // complex multiply : a*b
wiComplex wiComplexDiv    (wiComplex a, wiComplex b);    // complex division : a/b
wiComplex wiComplexInv    (wiComplex b);                 // complex inverse  : 1/a
wiComplex wiComplexConj   (wiComplex a);                 // complex conjugate: a*
wiComplex wiComplexNeg    (wiComplex a);                 // negative complex : -a
wiComplex wiComplexScaled (wiComplex a, wiReal b);       // scaled complex   : a*b, b is real
wiComplex wiComplexExp    (wiReal a);                    // exp(j*a)
wiReal    wiComplexReal   (wiComplex a);                 // Re{a}
wiReal    wiComplexImag   (wiComplex a);                 // Im{a}
//
//  Miscellaneous Mathematical
//
wiStatus wiMath_Combinations(int n, int k, int *c);
wiStatus wiMath_BinomialExpansion(int n, double *b, double *p);
wiStatus wiMath_ComplexConvolve_Accumulate(int Ny, wiComplex y[], int Nx, wiComplex x[], int Nh, wiComplex h[]);
wiStatus wiMath_ComplexConvolve(int Ny, wiComplex y[], int Nx, wiComplex x[], int Nh, wiComplex h[]);
wiUInt wiMath_FletcherChecksum(wiUInt NumBytes, void *ptr, wiUInt *pZ);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

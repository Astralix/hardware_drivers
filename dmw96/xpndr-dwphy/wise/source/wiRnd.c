//--< wiRnd.c >------------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Number Generator
//  Implements binary, uniform, and Gaussian random number generators
//
//=================================================================================================

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "wiUtil.h"
#include "wiRnd.h"

typedef double wiRndReal_t; // internal floating-point type for the random number generator

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
typedef struct 
{
    int         IsSeeded;
    int         LastSeed;
    wiRndReal_t u[98];
    wiRndReal_t c;
    wiRndReal_t cd;
    wiRndReal_t cm;
    int         i97;
    int         j97;
    unsigned    nCount[2];

} wiRnd_State_t;

static wiRnd_State_t State[WISE_MAX_THREADS] = {{0}};

const wiRndReal_t cd =  7654321.0 / 16777216.0;
const wiRndReal_t cm = 16777213.0 / 16777216.0;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define STATUS(Status)  WICHK((Status))
#define XSTATUS(Status) if (STATUS(Status)<0) return WI_ERROR_RETURNED;

// ================================================================================================
// FUNCTION  : wiRnd_State()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiRnd_State_t * wiRnd_State(void)
{
    return State + wiProcess_ThreadIndex();
}
// end of wiRnd_State()

// ================================================================================================
// FUNCTION  : wiRnd_Core()
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a uniform random sequence with 0 <= x <= 1.0
// Parameters: n -- Length of the array
//             x -- Array into which the random samples are placed
// ================================================================================================
wiRndReal_t wiRnd_GeneratorCore(wiRnd_State_t *pState)
{
    wiRndReal_t uni;

    uni = pState->u[pState->i97] - pState->u[pState->j97];
    if (uni < 0.0) uni += 1.0;

    pState->u[pState->i97] = uni;
    pState->i97--; if (pState->i97 == 0) pState->i97 = 97;
    pState->j97--; if (pState->j97 == 0) pState->j97 = 97;

    pState->c -= cd;
    if (pState->c < 0.0) pState->c += cm;

    uni -= pState->c;
    if (uni < 0.0) uni += 1.0;

    return uni;
}
// end of wiRnd_GeneratorCore(pState)

// ================================================================================================
// FUNCTION  : wiRnd_GetState()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns the current state of the random number generator as the original seed and
//             the sample offset from that seed.     
// Parameters: seed  -- Seed from 0...942377568 (-1 for random seed)
//             n1,n0 -- Offset to count
// ================================================================================================
wiStatus wiRnd_GetState(int *seed, unsigned int *n1, unsigned int *n0)
{
    wiRnd_State_t *pState = wiRnd_State();

    *seed = pState->LastSeed;
    *n1   = pState->nCount[1];
    *n0   = pState->nCount[0];
    return WI_SUCCESS;
}
// end of wiRnd_GetState()

// ================================================================================================
// FUNCTION  : wiRnd_InitThread()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the random number generator. NOTE: The seed variable can have a value 
//             from 0 to 942,377,568. Each seed can generate a unique sequence of ~10^30 values.
// Parameters: seed  -- Seed from 0...942377568
//             n1, n0 -- Offset to count
// ================================================================================================
wiStatus wiRnd_InitThread(int ThreadIndex, int seed, unsigned int n1, unsigned int n0)
{
    wiRndReal_t   s, t, uni;
    int       k, l, ii, m, jj, seed1, seed2;
    unsigned  i, j;
    wiRnd_State_t *pState = State + ThreadIndex;

    if (InvalidRange(seed, 0, WIRND_MAX_SEED)) return WI_ERROR_PARAMETER1;

    seed1 = seed % 31328;
    seed2 = seed / 31328;

    i = (seed1 / 177) % 177 + 2;
    j = (seed1 % 177) + 2;
    k = (seed2 / 169) % 178 + 1;
    l = (seed2 % 169);

    for (ii = 1; ii <= 97; ii++)
    {
        s = 0.0;
        t = 0.5;
        for (jj = 1; jj <= 24; jj++)
        {
            m = (((i*j) % 179)*k) % 179;
            i = j;
            j = k;
            k = m;
            l = (53*l + 1) % 169;
            if ((l*m) % 64 >= 32) s += t;
            t *= 0.5;
        }
        pState->u[ii] = s;
    }
    pState->c   =   362436.0 / 16777216.0;
    pState->cd  =  7654321.0 / 16777216.0;
    pState->cm  = 16777213.0 / 16777216.0;
    pState->i97 = 97;
    pState->j97 = 33;

    pState->IsSeeded = WI_TRUE;
    pState->LastSeed = seed;

    for (i = 0; i < n1; i++) {
        for (j = 0; j != 0x80000000; j++) {
            uni = wiRnd_GeneratorCore(pState);
        }
    }
    for (j = 0; j < n0; j++) {
        uni = wiRnd_GeneratorCore(pState);
    }
    pState->nCount[1] = n1;
    pState->nCount[0] = n0;

    return WI_SUCCESS;
}
// end of wiRnd_InitThread()

// ================================================================================================
// FUNCTION  : wiRnd_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the random number generator. NOTE: The seed variable can have a value 
//             from 0 to 942,377,568. Each seed can generate a unique sequence of ~10^30 values.
// Parameters: seed  -- Seed from 0...942377568 (-1 for random seed)
//             n1,n0 -- Offset to count
// ================================================================================================
wiStatus wiRnd_Init(int seed, unsigned int n1, unsigned int n0)
{
    int ThreadIndex;

    if (InvalidRange(seed, -1, WIRND_MAX_SEED)) return WI_ERROR_PARAMETER1;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate a random seed (for a seed value of -1)
    //  The seed is generated from the system clock with resolution of seconds. Because
    //  each thread can have its own seed, the seed is offset by a scaled version of the 
    //  of thread index.
    //
    if (seed == -1)
    {
        time_t    gt = time(NULL);      // system clock time
        struct tm gmt = *gmtime(&gt);
        seed = 2678400*gmt.tm_mon + 86400*gmt.tm_mday + 3600*gmt.tm_hour + 60*gmt.tm_min + gmt.tm_sec;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Per-Thread Initialization
    //
    for (ThreadIndex = 0; ThreadIndex < WISE_MAX_THREADS; ThreadIndex++) {
        XSTATUS( wiRnd_InitThread(ThreadIndex, (seed + ThreadIndex) % WIRND_MAX_SEED, n1, n0) );
    }
    return WI_SUCCESS;
}
// end of wiRnd_Init()

// ================================================================================================
// FUNCTION  : wiRnd_LastSeed()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns the last seed value; if no seed has been provided, -1 is returned
// Parameters: none
// ================================================================================================
int wiRnd_LastSeed(void)
{
   return wiRnd_State()->LastSeed;
}
// end of wiRnd_LastSeed()

// ================================================================================================
// FUNCTION  : wiRnd_UpdateState()
// ------------------------------------------------------------------------------------------------
// Purpose   : Update the state of the random number generator
// Parameters: n - Number of random samples drawn
// ================================================================================================
void wiRnd_UpdateState(wiRnd_State_t *pState, int n)
{
    pState->nCount[0] += n;
    if (pState->nCount[0] > 0x80000000)
    {
        pState->nCount[1]++;
        pState->nCount[0] = pState->nCount[0] & 0x7FFFFFFF;
    }
}
// end of wiRnd_UpdateState()

// ================================================================================================
// FUNCTION  : wiRnd_Binary()
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a random binary {0,1} sequence
// Parameters: n -- Length of the array
//             x -- Array into which the random samples are placed
// ================================================================================================
wiStatus wiRnd_Binary(int n, int *x)
{
    int k;
    wiRnd_State_t *pState = wiRnd_State();

    if (InvalidRange(n, 1, 1<<29)) return WI_ERROR_PARAMETER1;
    if (!x) return WI_ERROR_PARAMETER2;

    if (!pState->IsSeeded) wiRnd_Init(-1, 0, 0); // automatic initialization

    for (k=0; k<n; k++) {
        x[k] = (int)(wiRnd_GeneratorCore(pState) + 0.5);  // convert to int by rounding
    }
    wiRnd_UpdateState(pState, n);

    return WI_SUCCESS;
}
// end of wiRnd_Binary()

// ================================================================================================
// FUNCTION  : wiRnd_Uniform()
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a uniform random sequence with 0 <= x <= 1.0
// Parameters: n -- Length of the array
//             x -- Array into which the random samples are placed
// ================================================================================================
wiStatus wiRnd_Uniform(int n, wiReal *x)
{
    int k;
    wiRnd_State_t *pState = wiRnd_State();

    if (InvalidRange(n, 1, 1<<29)) return WI_ERROR_PARAMETER1;
    if (!x) return WI_ERROR_PARAMETER2;

    if (!pState->IsSeeded) wiRnd_Init(-1, 0, 0); // automatic initialization

    for (k=0; k<n; k++) {
        x[k] = (wiReal)wiRnd_GeneratorCore(pState);
    }
    wiRnd_UpdateState(pState, n);

    return WI_SUCCESS;
}
// end of wiRnd_Uniform()

// ================================================================================================
// FUNCTION  : wiRnd_Normal()
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a zero-mean normal (Gaussian) distribution of random numbers.
// Parameters: n -- Length of the array
//             x -- Array into which the random samples are placed
//             s -- Standard deviation of the distribution
// ================================================================================================
wiStatus wiRnd_Normal(int n, wiReal *x, wiReal s)
{
    int  i, k, nn=0;
    wiRndReal_t z, r2, v[2];

    wiRnd_State_t *pState = wiRnd_State();

    if (InvalidRange(n, 1, 1<<29)) return WI_ERROR_PARAMETER1;
    if (!x) return WI_ERROR_PARAMETER2;
    if (InvalidRange(s, 0.0, 1.0E+12)) return WI_ERROR_PARAMETER3;

    if (!pState->IsSeeded) wiRnd_Init(-1, 0, 0); // automatic initialization

    for (k = 0; k < n; )
    {
        do {
            for (i = 0; i < 2; i++)
                v[i] = (wiRndReal_t)(2.0 * wiRnd_GeneratorCore(pState) - 1.0);

            r2 = (wiRndReal_t)((v[0]*v[0]) + (v[1]*v[1]));
            nn += 2;
        } while (r2 >= 1 || r2 == 0.0);
        z = (wiRndReal_t)(s * sqrt(-2.0*log(r2)/r2));
        x[k++] = (wiReal)(v[0] * z);
        if (k < n) x[k++] = (wiReal)(v[1] * z);
    }
    wiRnd_UpdateState(pState, nn);

    return WI_SUCCESS;
}
// end of wiRnd_Normal()

// ================================================================================================
// FUNCTION  : wiRnd_ComplexUniform()
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a uniform random sequence with 0 <= x <= 1.0
// Parameters: n -- Length of the array
//             x -- Array into which the random samples are placed
// ================================================================================================
wiStatus wiRnd_ComplexUniform(int n, wiComplex *x)
{
    int k;
    wiRnd_State_t *pState = wiRnd_State();

    if (InvalidRange(n, 1, 1<<29)) return WI_ERROR_PARAMETER1;
    if (!x) return WI_ERROR_PARAMETER2;

    if (!pState->IsSeeded) wiRnd_Init(-1, 0, 0); // automatic initialization

    for (k = 0; k < n; k++)
    {
        x[k].Real = (wiReal)wiRnd_GeneratorCore(pState);
        x[k].Imag = (wiReal)wiRnd_GeneratorCore(pState);
    }
    wiRnd_UpdateState(pState, 2*n);

    return WI_SUCCESS;
}
// end of wiRnd_ComplexUniform()

// ================================================================================================
// FUNCTION  : wiRnd_ComplexNormal()
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a zero-mean normal (Gaussian) distribution of random complex numbers.
// Parameters: n -- Length of the array
//             x -- Array into which the random samples are placed
//             s -- Standard deviation of the complex distribution
// ================================================================================================
wiStatus wiRnd_ComplexNormal(int n, wiComplex *x, wiReal s)
{
    int  i, k, nn = 0;
    double v[2], z, r2;

    wiRnd_State_t *pState = wiRnd_State();

    if (InvalidRange(n, 1, 1<<29)) return WI_ERROR_PARAMETER1;
    if (!x) return WI_ERROR_PARAMETER2;
    if (InvalidRange(s, 0.0, 1.0E+12)) return WI_ERROR_PARAMETER3;

    if (!pState->IsSeeded) wiRnd_Init(-1, 0, 0); // automatic initialization

    s = s / sqrt(2.0); // scale standard deviation to correspond to each quadrature element

    for (k=0; k<n; k++)
    {
        do {
            for (i = 0; i < 2; i++)
                v[i] = (2.0 * wiRnd_GeneratorCore(pState) - 1.0);

            r2 = (v[0]*v[0]) + (v[1]*v[1]);
            nn += 2;
        } while (r2 >= 1 || r2 == 0.0);
        z = (s * sqrt(-2.0*log(r2)/r2));

        x[k].Real = (wiReal)(v[0] * z);
        x[k].Imag = (wiReal)(v[1] * z);
    }
    wiRnd_UpdateState(pState, nn);

    return WI_SUCCESS;
}
// end of wiRnd_ComplexNormal()

// ================================================================================================
// FUNCTION  : wiRnd_AddComplexNormal()
// ------------------------------------------------------------------------------------------------
// Purpose   : Adds a single zero-mean normal random variable to the complex signal
// Parameters: x -- Signal to which the random value is added
//             s -- Standard deviation of the complex distribution
// ================================================================================================
void wiRnd_AddComplexNormal(wiComplex *x, wiReal s)
{
    wiRndReal_t z, r2, vR, vI;
    wiRnd_State_t *pState = wiRnd_State();
    int n = 0;

    do {
        n += 2;
        vR = 2*wiRnd_GeneratorCore(pState) - 1;
        vI = 2*wiRnd_GeneratorCore(pState) - 1;
        r2 = (vR*vR + vI*vI);
    } while (r2>=1 || r2==0.0);

    wiRnd_UpdateState(pState, n);
    z = (wiRndReal_t)(s * sqrt(-log(r2)/r2));
    x->Real += (wiReal)(vR * z);
    x->Imag += (wiReal)(vI * z);
}
// end of wiRnd_AddComplexNormal()

// ================================================================================================
// FUNCTION  : wiRnd_UniformSample()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal wiRnd_UniformSample(void)
{
    wiReal x;

    wiRnd_Uniform(1, &x);
    return x;
}
// end of wiRnd_UniformSample()

// ================================================================================================
// FUNCTION  : wiRnd_NormalSample()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal wiRnd_NormalSample(wiReal StdDev)
{
    wiReal x;

    wiRnd_Normal(1, &x, StdDev);
    return x;
}
// end of wiRnd_NormalSample()


//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// ================================================================================================
// ADDITIONAL DOCUMENTATION
// ________________________
//
// This random number generator originally appeared in "Toward a Universal Random Number Generator" 
// by George Marsaglia and Arif Zaman.
// Florida State University Report: FSU-SCRI-87-50 (1987)
//
// It was later modified by F. James and published in "A Review of Pseudo-random Number Generators"
//
// THIS IS THE BEST KNOWN RANDOM NUMBER GENERATOR AVAILABLE.
//   (However, a newly discovered technique can yield a period of 10^600.
//    But that is still in the development stage.)
//
// It passes ALL of the tests for random number generators and has a period of 2^144, 
// is completely portable (gives bit identical results on all machines with at least 
// 24-bit mantissas in the floating point representation).
//
// The algorithm is a combination of a Fibonacci sequence (with lags of 97 and 33, and operation 
// "subtraction plus one, modulo one") and an "arithmetic sequence" (using subtraction).
// ________________________________________________________________________________________________
//
// The original was a FORTRAN program posted by David LaSalle of Florida State University. 
// It was translated to 'C' by Jim Butler and later rewritten for use with SIM and WiSE 
// by Barrett Brickner.
// ================================================================================================

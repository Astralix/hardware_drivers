//
//  wiRnd.h -- WISE Random Number Generator
//
//  The random number generators are based on an algorithm from FSU. Credits and references are
//  provided in comments in the source code. The generators here have been tested and are superior
//  to most others including the rand() function in the C library and the Numerical Recipies in C.
//

#ifndef __WIRND_H
#define __WIRND_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

#define WIRND_MAX_SEED 942377568

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=================================================================================================

//------ Utility Functions ------------------------------------------------------------------------
wiStatus wiRnd_SetVerbose(wiBoolean Verbose);
wiStatus wiRnd_Init(int seed, unsigned int n1, unsigned int n0);
wiStatus wiRnd_InitThread(int ThreadIndex, int seed, unsigned int n1, unsigned int n0);
int      wiRnd_LastSeed(void);
wiStatus wiRnd_GetState(int *seed, unsigned int *n1, unsigned int *n0);

//------ Random Number Arrays ---------------------------------------------------------------------
wiStatus wiRnd_Binary          (int n, int       *x);
wiStatus wiRnd_Uniform         (int n, wiReal    *x);
wiStatus wiRnd_Normal          (int n, wiReal    *x, wiReal StdDev);
wiStatus wiRnd_ComplexUniform  (int n, wiComplex *x);
wiStatus wiRnd_ComplexNormal   (int n, wiComplex *x, wiReal StdDev);

//------ Random Number Samples --------------------------------------------------------------------
wiReal   wiRnd_UniformSample(void);
wiReal   wiRnd_NormalSample(wiReal StdDev);

void     wiRnd_AddComplexNormal(wiComplex *x, wiReal s);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

//
//  wiFilter.h -- Digital Filter Library
//  Copyright 2001 Bermai, Inc. All Rights Reserved.
//      Also Copyright 1998 Barrett Brickner. All rights reserved.
//
//  This module contains routines for designing and implementing a Butterworth
//  low pass filter in sampled-time domain.

#ifndef __WIFILTER_H
#define __WIFILTER_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  ANALOG-TO-DIGITAL TRANSFORM TYPES
//=================================================================================================
#define WIFILTER_IMPULSE_INVARIANCE 1
#define WIFILTER_BILINEAR_TRANSFORM 2

//=================================================================================================
//  FILTER PROTOTYPES
//=================================================================================================
#define WIFILTER_BUTTERWORTH 0

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
wiStatus wiFilter_IIR             (int N, double b[], double a[], int Nx,    double x[],    double y[],    double w0[]);
wiStatus wiFilter_IIR_ComplexInput(int N, double b[], double a[], int Nx, wiComplex x[], wiComplex y[], wiComplex w0[]);

wiStatus wiFilter_Butterworth(int N, double fc, double b[], double a[], int method);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

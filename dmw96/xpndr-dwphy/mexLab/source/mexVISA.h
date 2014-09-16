/*
    mexVISA.h
*/

#ifndef __MEXVISA_H
#define __MEXVISA_H

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

#include "windows.h"
#include "mex.h"
#include "visa.h"

//=======================================================================================
//  VISA Data Type Definitions
//=======================================================================================
#define TYPE_ViStatus     0x0001
#define TYPE_ViBoolean    0x0002
#define TYPE_ViInt8       0x0008
#define TYPE_ViInt16      0x0016
#define TYPE_ViInt32      0x0032
#define TYPE_ViUInt8      0x0108
#define TYPE_ViUInt16     0x0116
#define TYPE_ViUInt32     0x0132
#define TYPE_ViSession    0x1000
#define TYPE_ViString     0x2000
#define TYPE_ViBusAddress 0x3001
#define TYPE_ViBusSize    0x3002
#define TYPE_ViVersion    0x4001
#define TYPE_ViAccessMode 0x4002
#define TYPE_ViEventType  0x4003
#define TYPE_ViJobId      0x4004

//=======================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=======================================================================================
ViStatus mexLab_GetViRM(ViSession *rmSession);
int mexLAB_VISA(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
//---------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//---------------------------------------------------------------------------------------
//--- End of File -----------------------------------------------------------------------

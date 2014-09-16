/*
    wiType.h -- Common type definitions for WISE
    Copyright 2001 Bermai, 2005 DSP Group, Inc. All rights reserved.

    This module contains definitions and variable definitions. Many of these are not
    strictly necessary when the simulator is built as a standalone executable or MEX
    library, but are included to provide the interfacing structure necessary to build
    the simulator as a distributable DLL.

    DO NOT MODIFY THIS FILE.
*/

#ifndef __WITYPE_H
#define __WITYPE_H

//==========================================================================
//  DEFAULT FUNCTION CALLING CONVENTION
//==========================================================================
#ifdef WIN32
#define WIAPI  __cdecl
#define CFUNC  __cdecl
#else
#define WIAPI
#define CFUNC
#endif

//==========================================================================
//  INCLUDE NECESSARY HEADER FILES
//==========================================================================
#include <stddef.h>  // required for size_t definition

//==========================================================================
//  STRUCTURE PACKING
//  WIPACK defines the WISE libraries' default structure packing value. The
//  packing is specified explicitly to prevent problems due to inconsistent
//  compiler settings.
//==========================================================================
#define WIPACK 8

//==========================================================================
//  DISABLE SPECIFIC MSVC++ WARNINGS
//  4201 -- "nonstandard extension used: nameless struct/union
//==========================================================================
#pragma warning(disable:4201)

//==========================================================================
//  DATA TYPES
//==========================================================================
#define _WI_PTR *

//--- Basic Types -------------------------------
typedef unsigned char    wiByte;
typedef   signed int     wiInt;
typedef unsigned int     wiUInt;
typedef          double  wiReal;
typedef          int     wiBoolean;
typedef     char _WI_PTR wiString;
typedef     void _WI_PTR wiAddr;

//--- Complex Real Types ------------------------
#pragma pack(WIPACK)
typedef struct {
    wiReal Real;
    wiReal Imag;
} wiComplex;
#pragma pack()

//--- Bitfield Assignment Structure-----------------------------------------
//--- Bitfield assignments are not defined under ANSI 'C'. The stated 
//--- behavior is guaranteed only for Microsoft Visual C++ w/Windows on
//--- Intel Pentium processors. Also tested correct using gcc on Intel based
//--- Linux workstations.
/*
#pragma pack(WIPACK)
typedef struct         
{
    unsigned int b0 :1; // Bit access. The individual bits in the WORD are
    unsigned int b1 :1; // accessible and number with the LSB being zero.
    unsigned int b2 :1;
    unsigned int b3 :1;
    unsigned int b4 :1;
    unsigned int b5 :1;
    unsigned int b6 :1;
    unsigned int b7 :1;
    unsigned int b8 :1;
    unsigned int b9 :1;
    unsigned int b10:1;
    unsigned int b11:1;
    unsigned int b12:1;
    unsigned int b13:1;
    unsigned int b14:1;
    unsigned int b15:1;
    unsigned int b16:1;
    unsigned int b17:1;
    unsigned int b18:1;
    unsigned int b19:1;
    unsigned int b20:1;
    unsigned int b21:1;
    unsigned int b22:1;
    unsigned int b23:1;
    unsigned int b24:1;
    unsigned int b25:1;
    unsigned int b26:1;
    unsigned int b27:1;
    unsigned int b28:1;
    unsigned int b29:1;
    unsigned int b30:1;
    unsigned int b31:1;
} wiBitField32Struct;
#pragma pack()
*/

#define wiBitField32Struct \
struct { \
    unsigned int b0 :1; \
    unsigned int b1 :1; \
    unsigned int b2 :1; \
    unsigned int b3 :1; \
    unsigned int b4 :1; \
    unsigned int b5 :1; \
    unsigned int b6 :1; \
    unsigned int b7 :1; \
    unsigned int b8 :1; \
    unsigned int b9 :1; \
    unsigned int b10:1; \
    unsigned int b11:1; \
    unsigned int b12:1; \
    unsigned int b13:1; \
    unsigned int b14:1; \
    unsigned int b15:1; \
    unsigned int b16:1; \
    unsigned int b17:1; \
    unsigned int b18:1; \
    unsigned int b19:1; \
    unsigned int b20:1; \
    unsigned int b21:1; \
    unsigned int b22:1; \
    unsigned int b23:1; \
    unsigned int b24:1; \
    unsigned int b25:1; \
    unsigned int b26:1; \
    unsigned int b27:1; \
    unsigned int b28:1; \
    unsigned int b29:1; \
    unsigned int b30:1; \
    unsigned int b31:1; \
}

//--- SIGNED Digital Word Type --------------------------------------
#pragma pack(WIPACK)
typedef union          
{
    signed int word;
    signed int w1 : 1;  // The following are 2's complement signed integers
    signed int w2 : 2;  // with the specified width. If an assignment is made
    signed int w3 : 3;  // to the "word" field, then all the individual w*
    signed int w4 : 4;  // fields are valid to the stated word width.
    signed int w5 : 5;
    signed int w6 : 6;
    signed int w7 : 7;
    signed int w8 : 8;
    signed int w9 : 9;
    signed int w10:10;
    signed int w11:11;
    signed int w12:12;
    signed int w13:13;
    signed int w14:14;
    signed int w15:15;
    signed int w16:16;
    signed int w17:17;
    signed int w18:18;
    signed int w19:19;
    signed int w20:20;
    signed int w21:21;
    signed int w22:22;
    signed int w23:23;
    signed int w24:24;
    signed int w25:25;
    signed int w26:26;
    signed int w27:27;
    signed int w28:28;
    signed int w29:29;
    signed int w30:30;
    signed int w31:31;
    signed int w32:32;
    unsigned int uword;
    unsigned int bit:1;
    wiBitField32Struct;
} wiWORD;
#pragma pack()

//--- UN-SIGNED Digital Word Type -----------------------------------
#pragma pack(WIPACK)
typedef union          
{
    unsigned int word;
    unsigned int w1 : 1; // The following are unsigned integers. If
    unsigned int w2 : 2; // an assignment is made to the "word" field,
    unsigned int w3 : 3; // then all the individual w* fields are
    unsigned int w4 : 4; // valid to the stated word width.
    unsigned int w5 : 5;
    unsigned int w6 : 6;
    unsigned int w7 : 7;
    unsigned int w8 : 8;
    unsigned int w9 : 9;
    unsigned int w10:10;
    unsigned int w11:11;
    unsigned int w12:12;
    unsigned int w13:13;
    unsigned int w14:14;
    unsigned int w15:15;
    unsigned int w16:16;
    unsigned int w17:17;
    unsigned int w18:18;
    unsigned int w19:19;
    unsigned int w20:20;
    unsigned int w21:21;
    unsigned int w22:22;
    unsigned int w23:23;
    unsigned int w24:24;
    unsigned int w25:25;
    unsigned int w26:26;
    unsigned int w27:27;
    unsigned int w28:28;
    unsigned int w29:29;
    unsigned int w30:30;
    unsigned int w31:31;
    unsigned int w32:32;
    unsigned int bit:1;
    wiBitField32Struct;
} wiUWORD;
#pragma pack()

//--- Binary Signal Delay Line Type ---------------------------------
#pragma pack(WIPACK)
typedef union          
{
    struct {
        unsigned int delay_0T : 1;
        unsigned int delay_1T : 1;
        unsigned int delay_2T : 1;
        unsigned int delay_3T : 1;
        unsigned int delay_4T : 1;
        unsigned int delay_5T : 1;
        unsigned int delay_6T : 1;
        unsigned int delay_7T : 1;
        unsigned int delay_8T : 1;
        unsigned int delay_9T : 1;
        unsigned int delay_10T: 1;
        unsigned int delay_11T: 1;
        unsigned int delay_12T: 1;
        unsigned int delay_13T: 1;
        unsigned int delay_14T: 1;
        unsigned int delay_15T: 1;
        unsigned int delay_16T: 1;
        unsigned int delay_17T: 1;
        unsigned int delay_18T: 1;
        unsigned int delay_19T: 1;
        unsigned int delay_20T: 1;
        unsigned int delay_21T: 1;
        unsigned int delay_22T: 1;
        unsigned int delay_23T: 1;
        unsigned int delay_24T: 1;
        unsigned int delay_25T: 1;
        unsigned int delay_26T: 1;
        unsigned int delay_27T: 1;
        unsigned int delay_28T: 1;
        unsigned int delay_29T: 1;
        unsigned int delay_30T: 1;
        unsigned int delay_31T: 1;
    };
    unsigned int word;
    unsigned int bit:1;
    wiBitField32Struct;
} wiBitDelayLine;
#pragma pack()

//--- 'D' Registers -------------------------------------------------
typedef struct {wiWORD  D; wiWORD  Q;} wiReg;
typedef struct {wiUWORD D; wiUWORD Q;} wiUReg;

#undef _WI_PTR

//==========================================================================
//  DATA TYPE LABELS
//==========================================================================
#define WITYPE_ADDR    0
#define WITYPE_INT     1
#define WITYPE_UINT    2
#define WITYPE_REAL    3
#define WITYPE_COMPLEX 4
#define WITYPE_STRING  5
#define WITYPE_BOOLEAN 6

//==========================================================================
//  STATUS CODE TYPE
//  wiStatus is the return value type for most library functions.
//==========================================================================
typedef int wiStatus;

//==========================================================================
//  PRINTF FUNCTION TYPE
//  Define a pointer to a function having the same prototype as "printf"
//==========================================================================
typedef int (*wiPrintfFn)(const char *__format, ...);

//==========================================================================
//  OUTPUT/MESSAGE FUNCTION TYPE
//  Define a pointer to a function used to output messages
//==========================================================================
typedef wiStatus (*wiMessageFn)(char *__format, ...);

//==========================================================================
//  MEMORY MANAGEMENT FUNCTION TYPES
//  Define a pointer to a function having the same prototype as the standard
//  memory management functions.
//==========================================================================
typedef void *(CFUNC *wiMallocFn) (size_t __size);
typedef void *(CFUNC *wiCallocFn) (size_t __nitems, size_t __size);
typedef void *(CFUNC *wiReallocFn)(void * __block, size_t __size);
typedef void  (CFUNC *wiFreeFn)   (void * __block);

//==========================================================================
//  MISCELLANEOUS DEFINITIONS
//==========================================================================
#define WI_NULL   0
#define WI_FALSE  0
#define WI_TRUE   1

//==========================================================================
//  DEFINE WIDLL FOR THIS BUILD
//  This is used export functions during a DLL build and to import them when
//  the library is included in an application.
//==========================================================================
/*
#if defined(BUILD_WISE)
    #define WIDLL __declspec(dllexport)
#elif defined(NO_WIDLL)
    #define WIDLL
#else
    #define WIDLL __declspec(dllimport)
#endif
*/

#define WIDLL

#endif
//--------------------------------------------------------------------------
//--- End of File ----------------------------------------------------------


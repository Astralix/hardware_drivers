//
//  Verona_dpdBlocks.h -- Digital Predistortion Module
//  Copyright 2010 DSP Group, Inc. All rights reserved.
//

#ifndef __VERONA_DPDBLOCKS_H
#define __VERONA_DPDBLOCKS_H

// #include "dpdMath.h"
#ifndef __VERONA_DPDMATH_H
#define __DPDMATH_H

// #include "dpdType.h"

#ifndef __DPDTYPE_H
#define __DPDTYPE_H



//typedef unsigned	int dpdStatus;
typedef	signed		int	dpdInt;
typedef unsigned	int	dpdUInt;

typedef enum {dpd_failure, dpd_success} dpdStatus;


typedef union {
    dpdInt dat[3];
    struct
{
    dpdInt	Real;
    dpdInt	Imag;
    dpdUInt	Precision;
    };
    struct
{
    dpdInt Real;
    dpdInt Imag;
    dpdInt Precision;
    } complexValue;
} dpdComplexInt;



typedef union {
    dpdInt dat[2];
    struct {
        dpdInt	Value;
        dpdUInt Precision;
    };
    struct
{
    dpdInt Value;
    dpdUInt Precision;
    }realValue;
} dpdRealInt;

#endif // _DPDTYPE_H


#define COMPLEX_MULT_REAL(A,B) A->Real*B->Real - A->Imag*B->Imag
#define COMPLEX_MULT_IMAG(A,B) A.Real*B.Imag + A.Imag*B.Real

#define MAX(A,B)((A)>(B)?(A):(B))

#define  DPD_SET_COMPLEX_INT(A, B, C) \
do{(A)->Real = B; (A)->Imag = C; /*restrictPrecComplex(A);*/} while(0)

#define  DPD_SET_COMPLEX_INT_PR(A, B, C, D) \
do{(A)->Real = B; (A)->Imag = C; (A)->Precision = D; /*restrictPrecComplex(A);*/} while(0)

#define DPD_SET_BUFFER(B, SOURCE, PREC, SIZE) \
	{int ijij; for(ijij=0; (unsigned int)ijij<(unsigned int)SIZE; ijij++){	\
		DPD_SET_COMPLEX_INT_PR(&B[ijij], SOURCE[2*ijij], SOURCE[2*ijij+1], PREC);	\
	}}


#define SET_SIGN(Value, NORM) ((Value+(NORM>>1))%NORM)-(NORM>>1)

#define DPD_COMPLEX_REAL(z)     ((z).dat[0])
#define DPD_COMPLEX_IMAG(z)     ((z).dat[1])
#define DPD_COMPLEX_PREC(z)     ((z).dat[2])

// #define SATURATE(A, LIMIT)		{A = A>LIMIT? LIMIT : A; A = A<-LIMIT? -LIMIT : A;}


// #define DPD_COMPLEX_REAL(z)     ((z).Real)

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// Protypes of basic functions
dpdComplexInt Verona_dpdComplexMultInt(dpdComplexInt, dpdComplexInt);
dpdComplexInt Verona_dpdComplexAddInt(dpdComplexInt, dpdComplexInt);
dpdComplexInt Verona_dpdComplexShiftR(dpdComplexInt, dpdUInt);
dpdComplexInt Verona_dpdComplexQuadInt(dpdComplexInt);

dpdComplexInt Verona_dpdRealMultInt(dpdComplexInt, dpdRealInt);
dpdRealInt Verona_dpdRealShiftR(dpdRealInt, dpdUInt);

dpdStatus Verona_restrictPrecComplex(dpdComplexInt *);
dpdStatus    Verona_restrictPrecReal(dpdRealInt *);

dpdComplexInt Verona_dpdComplexInit1(dpdRealInt, dpdRealInt);
dpdComplexInt Verona_dpdComplexInit2(dpdInt, dpdInt, dpdUInt);

dpdRealInt Verona_dpdRealInit(dpdInt, dpdUInt);
dpdRealInt Verona_dpdRealRealMultInt(dpdRealInt, dpdRealInt);

dpdRealInt Verona_dpdRealAddInt(dpdRealInt, dpdRealInt);

dpdComplexInt Verona_dpdComplexCopyInt(dpdComplexInt);
dpdRealInt Verona_dpdRealCopyInt(dpdRealInt);

dpdStatus Verona_setComplexIntPrecision(dpdComplexInt *, dpdUInt);
dpdStatus Verona_setRealIntPrecision(dpdRealInt *, dpdUInt);

dpdInt Verona_convert_to_signed(dpdInt, int);
int Verona_SATURATE(int, int);    // +LIMIT, -LIMIT
int Verona_SATURATE2(int, int);   // +LIMIT, -(LIMIT+1)



#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif


#endif // __DPDMATH_H



typedef struct
{
    dpdComplexInt *delay_line;
    dpdComplexInt *b_coeff;
    dpdComplexInt *a_coeff;
    int         size;
    int         pointer;
} DELAY_LINE_COMPLEX;


#define INIT_DELAY_LINE(DL, D,B,A,S) \
        { \
        DL->delay_line = D;  \
        DL->b_coeff = B;     \
        DL->a_coeff = A;     \
        DL->size = S;        \
        DL->pointer = 0;     \
        }

#define INIT_VFP_FIR(FIR, T,P) \
        { \
        FIR->taps = T;  \
        FIR->precision = P;     \
		FIR->fir = (DELAY_LINE_COMPLEX *)NULL; \
        }


// FIR Parameter Structure
typedef struct
{
	dpdUInt taps;
	dpdUInt precision;
	DELAY_LINE_COMPLEX *fir;
} dpdFIR;

// DPD Parameter Structure
typedef struct
{
    dpdUInt num_of_firs;
    struct
	{
        struct
		{
			dpdUInt dpd_in_precision;
			dpdUInt shift_right1;
			dpdUInt shift_right2;
			dpdUInt shift_right3;
			dpdUInt shift_right4;
        } dpdHigherOrder;
        //	dpdFIR vfir_p1;
        //	dpdFIR vfir_p3;
        //	dpdFIR vfir_p5;
        dpdFIR **vfir;
        struct
		{
        /*
        dpdUInt shift_right_p1;
        dpdUInt shift_right_p2;
        dpdUInt shift_right_p3;
         */
			dpdUInt shift_right[10];
        } firNormalization;
    } VFP;
} dpdBlock;




#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif



void Verona_clean_delay_line(DELAY_LINE_COMPLEX *);
dpdStatus Verona_dpdHighOrder(dpdBlock *, dpdComplexInt **, int, int);

dpdStatus Verona_complex_fir(DELAY_LINE_COMPLEX *, dpdComplexInt *, dpdComplexInt *, int);


dpdStatus Verona_dpdBlock_Init(int);
dpdStatus Verona_dpdVFP(int , dpdComplexInt *, int, dpdComplexInt * );
dpdStatus Verona_dpdVFIRs_Init(dpdBlock *);
dpdBlock *Verona_get_dpdBlock(int );
dpdStatus Verona_dpdBlock_CleanFilters(int);
dpdStatus Verona_dpdBlock_Close(int);
dpdStatus Verona_dpdBlock_SetFilterCoeff(int, int);


#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif


#endif // __VERONA_DPDBLOCKS_H

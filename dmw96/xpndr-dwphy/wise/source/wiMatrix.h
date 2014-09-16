//
//  wiMatrix.h -- WISE Matrix Library
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.   
//
//  This module contains a variety of manipulations implemented in C for complex matrices, including 
//  some basic operations, matrix inversion, QR decomposition, and other useful tools. Complex column
//  and row vectors are treated as special cases, in which one of the two dimensions has size 1. 
//

#ifndef __WIMATRIX_H
#define __WIMATRIX_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// Complex Matrix Structure (Type)
//
typedef struct {
    wiInt       row;               // number of rows (used)
    wiInt       col;               // number of columns per row (used)
    wiInt       RowsAllocated;     // total number of rows allocated (val)
    wiInt       ElementsAllocated; // total number of elements allocated (baseptr)
    wiComplex  *baseptr;           // pointer to the base address for the element array
    wiComplex **val;               // pointer to a two-dimension array
    wiInt       ThreadIndex;       // used internally to manage multi-threaded memory use
    void       *pSmallMatrix;      // pointer to a private data structure (for internal use)
    
} wiCMat;
//
//  Module Utilities  /////////////////////////////////////////////////////////////////////////////
//
wiStatus wiMatrix_Init(void);
wiStatus wiMatrix_Close(void);
//
//  Complex Matrix (wiCMat) Functions  ////////////////////////////////////////////////////////////
//
wiStatus wiCMatInit  (wiCMat *x, wiInt row, wiInt col);       // Matrix memory initialization
wiStatus wiCMatReInit(wiCMat *x, wiInt row, wiInt col);       // Re-initialize existing matrix
wiStatus wiCMatFree  (wiCMat *x);                             // Free matrix memory 

wiStatus wiCMatGetElement(wiCMat *x, wiComplex *a, wiInt i, wiInt j); // Get (i,j)th element of x
wiStatus wiCMatSetElement(wiCMat *x, wiComplex *a, wiInt i, wiInt j); // Set (i,j)th element of x

wiStatus wiCMatSetArray2Mtx(wiCMat *y, wiComplex *x[]); // copy double array to matrix
wiStatus wiCMatSetMtx2Array(wiCMat *y, wiComplex *x[]); // copy matrix to double array 

wiStatus wiCMatDelRow(wiCMat *x, wiInt row); // Delete a row from matrix 
wiStatus wiCMatDelCol(wiCMat *x, wiInt col); // Delete a column from matrix

wiStatus wiCMatSetZero(wiCMat *x);// Set all elements of x to zero
wiStatus wiCMatSetIden(wiCMat *x);// Set all diagonal elements to one and others to zero

wiStatus wiCMatCopy(const wiCMat *source, wiCMat *destination); // Copy source to destination

wiStatus wiCMatTrans(wiCMat *y, wiCMat *x); // Compute the transpose of x
wiStatus wiCMatConjTrans(wiCMat *y, wiCMat *x); // Compute the conjugate transpose of x

wiStatus wiCMatScale(wiCMat *y, wiCMat *x, wiComplex scalar); // y = scalar * x 
wiStatus wiCMatScaleSelf(wiCMat *x, wiComplex scalar); // x = scalar * x 
wiStatus wiCMatScaleSelfReal(wiCMat *x, wiReal scalar); // x = scalar * x 

wiStatus wiCMatAdd(wiCMat *z, wiCMat *x, wiCMat *y); // z = x + y
wiStatus wiCMatSub(wiCMat *z, wiCMat *x, wiCMat *y); // z = x - y
wiStatus wiCMatAddSelf(wiCMat *y, wiCMat *x); // y = y + x 
wiStatus wiCMatScaleAdd(wiCMat *z, wiCMat *x,  wiCMat *y, 
                    wiComplex scalar_x, wiComplex scalar_y); // z = scalar_x * x + scalar_y * y
wiStatus wiCMatScaleAddSelf(wiCMat *y, wiCMat *x, wiComplex scalar_x); // y = y + scalar_x * x 

wiStatus wiCMatMul(wiCMat *z, wiCMat *x,  wiCMat *y);// z = x * y
wiStatus wiCMatMulSelfLeft(wiCMat *y, wiCMat *x);// y = x * y
wiStatus wiCMatMulSelfRight(wiCMat *y, wiCMat *x);// y = y * x

wiReal    wiCMatFrobeNormSquared(wiCMat *x); // z =      trace(x*x^H) 
wiReal    wiCMatFrobeNorm       (wiCMat *x); // z = sqrt(trace(x*x^H))
wiComplex wiCMatTrace(wiCMat *x);

wiStatus wiCMatGetSubMtx(wiCMat *x, wiCMat *x_sub, wiInt i1, wiInt j1, wiInt i2, wiInt j2); // Extract from x a submatrix and store it to x_sub
wiStatus wiCMatSetSubMtx(wiCMat *x, wiCMat *x_sub, wiInt i1, wiInt j1, wiInt i2, wiInt j2); // overwrite a submatrix of x with x_sub

wiStatus wiCMatSubCorner(wiCMat *y, wiCMat *x); // Substract from y a smaller x 
                                                // aligned at the right bottom, used in QR decomposition

wiStatus wiCMatSwapColumn(wiCMat *x, wiInt i, wiInt j);

wiStatus wiCMatQR(wiCMat *A, wiCMat *Q,  wiCMat *R);
                                        // QR decomposition, A: m-by-n, is supplose to be full column rank: m>=n
                                        // Q: m-by-n, R: n-by-n
wiStatus wiCMatAscendingSortQR(wiCMat *A, wiCMat *Q,  wiCMat *R, wiInt *p);
                                        // QR decomposition, A: m-by-n, is supplose to be full column rank: m>=n
                                        // Q: m-by-n, R: n-by-n  p: array of size n
wiStatus wiCMatQRWrapper(wiCMat *A, wiCMat *Q,  wiCMat *R, wiInt *P, wiInt ModifiedQR, wiInt Sorted );
wiStatus wiCMatComplexToReal(wiCMat *A, wiCMat *A1, wiInt DoubleColumn);

wiStatus wiCMatInv(wiCMat *InvA, wiCMat *A); // InvA = A^-1
wiStatus wiCMatDisplay(wiCMat *x); // Display contents of x row by row

wiStatus wiCMatKron(wiCMat *z, wiCMat *x,  wiCMat *y); // Compute the Kronecker product of x and y
wiStatus wiCMatCholesky(wiCMat *L, wiCMat *R);         // Compute the Cholesky decomposition of R as R=L^H * L;

wiStatus wiCMatMul3(wiCMat *A, wiCMat *U,  wiCMat *S,  wiCMat *V);// A = U * S * V*
wiStatus wiCMatSVD (wiCMat *U, wiCMat *S, wiCMat *V, wiCMat *A); // SVD: A = U S V*
//wiStatus wiCMatTestSVD(); // SVD: A = U S V*
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

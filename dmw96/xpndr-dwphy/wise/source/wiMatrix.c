//--< wiMatrix.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Matrix Operations
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "wiMath.h"
#include "wiMatrix.h"
#include "wiParse.h"
#include "wiRnd.h"
#include "wiUtil.h"

const double epsilon = 1.0e-15; // a constant very small value

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Small Matrix Definitions
//
//  Modules such as Phy11n make use of a large number of small matrices (those with dimensions on
//  the order of the number of antennas). To avoid excessive allocate-free cycles on the process 
//  heap that can lead to fragmentation, the module preallocates a certain number of small matrices
//  have a fixed size and maintains these and any others added later until the process is 
//  terminated. A list of free small matrix objects allows rapid assignment and deallocation of 
//  these as memory for wiCMat objects.
//
//  WICMAT_SMALL_DIM is the largest row and column dimension to qualify as "small"
//  WICMAT_INITIAL_SMALL_ALLOCATION is the number of elements to pre-allocate
//
#define WICMAT_SMALL_DIM 8
#define WICMAT_INITIAL_SMALL_ALLOCATION  100

typedef struct wiCMatSmallMatrix_S
{
    struct wiCMatSmallMatrix_S *NextAllocated; // pointer to the next item in the AllocatedList
    struct wiCMatSmallMatrix_S *NextFree;      // pointer to the next item in the FreeList

    wiComplex *p[WICMAT_SMALL_DIM];                    // matrix row pointers
    wiComplex  d[WICMAT_SMALL_DIM * WICMAT_SMALL_DIM]; // matrix element storage

    wiBoolean  Used;

} wiCMatSmallMatrix_t;

wiCMatSmallMatrix_t *pAllocatedList[WISE_MAX_THREADS] = {NULL}; // pointer to the list of small matrix items
wiCMatSmallMatrix_t *pFreeList[WISE_MAX_THREADS]      = {NULL}; // pointer to the list of free small matrix items

static wiBoolean ModuleIsInitialized = WI_FALSE;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

//=================================================================================================
//--  OTHER MACROS
//=================================================================================================
#ifndef SQR
#define SQR(x)   ((x)*(x))
#endif

#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#define max(a,b) (((a)>(b))? (a):(b))
#endif

//=================================================================================================
//  COMPLEX NUMBER OPERATIONS
//
//  COMPLEX_MULTIPLY(c,a,b)  ---> c = a * b
//=================================================================================================
#ifndef COMPELX_MULTIPLY
    #define COMPLEX_MULTIPLY(c,a,b)                           \
        {                                                     \
            c.Real = (a.Real * b.Real) - (a.Imag * b.Imag);   \
            c.Imag = (a.Real * b.Imag) + (a.Imag * b.Real);   \
        }                                                     \
    //  -------------------------------------------------------
#endif


// ================================================================================================
// FUNCTION  : wiCMatAddSmallMatrix()
// ------------------------------------------------------------------------------------------------
// Purpose   : Allocate a "Small Matrix" item and add it to the FreeList
// ================================================================================================
wiStatus wiCMatAddSmallMatrix(int i)
{
    wiCMatSmallMatrix_t *pSmallMatrix;

    WICALLOC( pSmallMatrix, 1, wiCMatSmallMatrix_t );

    pSmallMatrix->NextAllocated = pAllocatedList[i];
    pSmallMatrix->NextFree      = pFreeList[i];
    
    pAllocatedList[i] = pSmallMatrix;
    pFreeList[i]      = pSmallMatrix;

    return WI_SUCCESS;
}
// end of wiCMatAddSmalMatrix()


// ================================================================================================
// FUNCTION  : wiCMatInit()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the matrix item pointed to by pMatrix to contain a row x col sized matrix
//             of zero elements.
// ================================================================================================
wiStatus wiCMatInit(wiCMat *pMatrix, wiInt row, wiInt col)
{
    int i;
    int ThreadIndex = wiProcess_ThreadIndex();

    if (!ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);
    if (row <= 0) return STATUS(WI_ERROR_PARAMETER2);
    if (col <= 0) return STATUS(WI_ERROR_PARAMETER3);

    if ((row <= WICMAT_SMALL_DIM) && (col <= WICMAT_SMALL_DIM)) // Small Matrix //////////////
    {
        wiCMatSmallMatrix_t *pSmallMatrix;

        if (!pFreeList[ThreadIndex]) {
            XSTATUS(wiCMatAddSmallMatrix(ThreadIndex));
        }
        pSmallMatrix           = pFreeList[ThreadIndex];
        pFreeList[ThreadIndex] = pFreeList[ThreadIndex]->NextFree;
    
        pMatrix->val               = pSmallMatrix->p;
        pMatrix->baseptr           = pSmallMatrix->d;
        pMatrix->RowsAllocated     = WICMAT_SMALL_DIM;
        pMatrix->ElementsAllocated = WICMAT_SMALL_DIM * WICMAT_SMALL_DIM;
        pMatrix->pSmallMatrix      = pSmallMatrix;
        pSmallMatrix->Used         = WI_TRUE;

        memset(pMatrix->baseptr, 0, pMatrix->ElementsAllocated * sizeof(wiComplex  ));
        memset(pMatrix->val,     0, pMatrix->RowsAllocated     * sizeof(wiComplex *));
    }
    else // Large Matrix /////////////////////////////////////////////////////////////////////
    {
        int numel = row * col;

        WICALLOC( pMatrix->val,     row,   wiComplex * );
        WICALLOC( pMatrix->baseptr, numel, wiComplex   );

        pMatrix->RowsAllocated     = row;
        pMatrix->ElementsAllocated = numel;
        pMatrix->pSmallMatrix       = NULL;
    }
    for (i = 0; i < row; i++)
        pMatrix->val[i] = pMatrix->baseptr + (i * col);

    pMatrix->row = row;
    pMatrix->col = col;
    pMatrix->ThreadIndex = ThreadIndex;

    return WI_SUCCESS;
}
// end of wiCMatInit()

// ================================================================================================
// FUNCTION  : wiCMatReInit()
// ------------------------------------------------------------------------------------------------
// Purpose   : Reinitialize a matrix. If the existing matrix is too small, it is freed and re-
//             allocated; otherwise, the existing space is reformed to the new dimensions.
// ================================================================================================
wiStatus wiCMatReInit(wiCMat *pMatrix, wiInt row, wiInt col)
{
    int ThreadIndex = wiProcess_ThreadIndex();

    if (row <= 0) return STATUS(WI_ERROR_PARAMETER2);
    if (col <= 0) return STATUS(WI_ERROR_PARAMETER3);
    
    if (!pMatrix->val || 
       (row > pMatrix->RowsAllocated) || 
       (row*col > pMatrix->ElementsAllocated) ||
       (pMatrix->ThreadIndex != ThreadIndex) )
    {
        XSTATUS( wiCMatFree(pMatrix) );
        XSTATUS( wiCMatInit(pMatrix, row, col) );
    }
    else // re-use the existing space
    {
        int i;

        for (i = 0; i < row; i++)
            pMatrix->val[i] = pMatrix->baseptr + (i * col); 
    
        pMatrix->row = row;
        pMatrix->col = col;

        memset( pMatrix->baseptr, 0, pMatrix->ElementsAllocated * sizeof(wiComplex) );
    }
    return WI_SUCCESS;
}
// end of wiCMatReInit()

// ================================================================================================
// FUNCTION  : wiCMatFree()
// ------------------------------------------------------------------------------------------------
// Purpose   : Release memory used by the matrix referenced by pMatrix
// ================================================================================================
wiStatus wiCMatFree(wiCMat *pMatrix)
{
    wiInt ThreadIndex = wiProcess_ThreadIndex();

    if (pMatrix->val && (!ThreadIndex || ThreadIndex == pMatrix->ThreadIndex)) 
    {
        if (pMatrix->val)
        {
            if (pMatrix->pSmallMatrix == NULL)
            {
                WIFREE( pMatrix->baseptr );
                WIFREE( pMatrix->val );
            }
            else
            {
                wiCMatSmallMatrix_t *p = (wiCMatSmallMatrix_t *)(pMatrix->pSmallMatrix);

                pMatrix->baseptr = NULL;
                pMatrix->val     = NULL;
                p->Used          = WI_FALSE;

                p->NextFree = pFreeList[pMatrix->ThreadIndex];
                pFreeList[pMatrix->ThreadIndex] = p;
            }
        }
    }
    pMatrix->val = NULL;
    pMatrix->pSmallMatrix = NULL;
    pMatrix->ElementsAllocated  = 0;
    pMatrix->RowsAllocated      = 0;
    pMatrix->row = pMatrix->col = 0;
    return WI_SUCCESS;
}
// end of wiCMatFree()

// ================================================================================================
// FUNCTION  : wiMatrix_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMatrix_Init(void)
{
    int i, j;

    if (ModuleIsInitialized) return WI_ERROR_MODULE_ALREADY_INIT;
    ModuleIsInitialized = WI_TRUE;

    for (i = 0; i < WISE_MAX_THREADS; i++)
        for (j = 0; j < WICMAT_INITIAL_SMALL_ALLOCATION; j++)
            XSTATUS( wiCMatAddSmallMatrix(i) );

    return WI_SUCCESS;
}
// end of wiMatrix_Init()

// ================================================================================================
// FUNCTION  : wiMatrix_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMatrix_Close(void)
{
    int i;
    if (!ModuleIsInitialized) return WI_ERROR_MODULE_NOT_INITIALIZED;

    for (i = 0; i<WISE_MAX_THREADS; i++)
    {
        while (pAllocatedList[i])
        {
            wiCMatSmallMatrix_t *pNext = pAllocatedList[i]->NextAllocated;

            if (pAllocatedList[i]->Used) 
                wiUtil_WriteLogFile("Freeing small memory @x%08X not deallocated with wiCMatFree()\n",
                                    pAllocatedList[i]);

            WIFREE(pAllocatedList[i]);
            pAllocatedList[i] = pNext;
        }
        pFreeList[i] = NULL; 
    }
    return WI_SUCCESS;
}
// end of wiMatrix_Close()

/*============================================================================+
|  Function  : wiCMatGetElement()                                             |
|-----------------------------------------------------------------------------|
|  Purpose   : Retrieve the value of element (i,j)                            |
|  Parameters: x,a,i,j                                                        |
+============================================================================*/
wiStatus wiCMatGetElement(wiCMat *x, wiComplex *a, wiInt i, wiInt j)
{
    if (i<0 || i>=x->row || j<0 || j>=x->col) return STATUS(WI_ERROR_RANGE);
    *a = x->val[i][j];

    return WI_SUCCESS;
}
// end of wiCMatGetElement()

/*============================================================================+
|  Function  : wiCMatSetElement()                                             |
|-----------------------------------------------------------------------------|
|  Purpose   : set the value of element (i,j)                                 |
|  Parameters: x,a,i,j                                                        |
+============================================================================*/
wiStatus wiCMatSetElement(wiCMat *x, wiComplex *a, wiInt i, wiInt j)
{
    if (i<0 || i>=x->row || j<0 || j>=x->col) return STATUS(WI_ERROR_RANGE);
    x->val[i][j] = *a;

    return WI_SUCCESS;
}
// end of wiCMatSetElement()

/*============================================================================+
|  Function  : wiCMatSetArray2Mtx()                                           |
|-----------------------------------------------------------------------------|
|  Purpose   : Copy complex double arry x to matrix y                         |
|              Note: the type of x is wiComplex *x[]                          |
|  Parameters: y,x                                                            |
+============================================================================*/
wiStatus wiCMatSetArray2Mtx(wiCMat *y, wiComplex *x[])
{
    int i, j;

    for (i=0; i<y->row; i++)
        for (j=0; j<y->col; j++)
            y->val[i][j] = x[i][j];

    return WI_SUCCESS;
}
// end of wiCMatSetArray2Mtx()

/*============================================================================+
|  Function  : wiCMatSetMtx2Array()                                           |
|-----------------------------------------------------------------------------|
|  Purpose   : Copy matrix y to complex double arry x                         |
|              Note: the type of x is wiComplex *x[]                          |
|  Parameters: y,x                                                            |
+============================================================================*/
wiStatus wiCMatSetMtx2Array(wiCMat *y, wiComplex *x[])
{
    int i, j;

    for (i=0; i<y->row; i++)
        for (j=0; j<y->col; j++)
            x[i][j] = y->val[i][j];

    return WI_SUCCESS;
}
// end of wiCMatSetMtx2Array()

/*============================================================================+
|  Function  : wiCMatSetZero()                                                |
|-----------------------------------------------------------------------------|
|  Purpose   : Set all elements of x to 0                                     |
|  Parameters: x                                                              |
+============================================================================*/
wiStatus wiCMatSetZero(wiCMat *x)
{
    memset( x->baseptr, 0, x->ElementsAllocated * sizeof(wiComplex) );
    return WI_SUCCESS;
}
// end of wiCMatSetZero()

/*============================================================================+
|  Function  : wiCMatSetIden()                                                |
|-----------------------------------------------------------------------------|
|  Purpose   : Set diagonal to one and other to zero                          |
|  Parameters: x                                                              |
+============================================================================*/
wiStatus wiCMatSetIden(wiCMat *x)
{
    int i;
    memset( x->baseptr, 0, x->ElementsAllocated * sizeof(wiComplex) );

    // set diagonal elements to one
    for (i=0; i<min(x->col, x->row); i++)
        x->val[i][i].Real = 1;

    return WI_SUCCESS;
}
// end of wiCMatSetIden()

/*============================================================================+
|  Function  : wiCMatCopy()                                                   |
|-----------------------------------------------------------------------------|
|  Purpose   : Copy the content of source to desination                       |
|  Parameters: source, destination                                            |
+============================================================================*/
wiStatus wiCMatCopy(const wiCMat *source, wiCMat *destination)
{
    int i;
    size_t rowsize = source->col * sizeof(wiComplex);

    // check size mismatch
    if (source->row!=destination->row || source->col!=destination->col)
        return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH);

    for (i=0; i<source->row; i++)
        memcpy(destination->val[i], source->val[i], rowsize);

    return WI_SUCCESS;
}
// end of wiCMatCopy()

/*============================================================================+
|  Function  : wiCMatDelRow()                                                 |
|-----------------------------------------------------------------------------|
|  Purpose   : Delete a tow from matrix, but the memeory allocated            |
|              for matrix is unchanged                                        |
|  Parameters: x, row                                                         |
+============================================================================*/
wiStatus wiCMatDelRow(wiCMat *x, wiInt row)
{
    int i, j;

    if (row<0 || row>=x->row) return STATUS(WI_ERROR_PARAMETER2);

    for (i=row; i<x->row-1; i++)
        for (j=0; j<x->col; j++)
            x->val[i][j] = x->val[i+1][j];

    x->row -= 1;

    return WI_SUCCESS;
}
// end of wiCMatDelRow()

/*============================================================================+
|  Function  : wiCMatDelCol()                                                 |
|-----------------------------------------------------------------------------|
|  Purpose   : Delete a column from matrix, but the memeory allocated         |
|              for matrix is unchanged                                        |
|  Parameters: x, col                                                         |
+============================================================================*/
wiStatus wiCMatDelCol(wiCMat *x, wiInt col)
{
    int i, j;

    if (col<0 || col>=x->col) return STATUS(WI_ERROR_PARAMETER2);

    for (j=col; j<x->col-1; j++)
        for (i=0; i<x->row; i++)
            x->val[i][j] = x->val[i][j+1];

    x->col -= 1;

    return WI_SUCCESS;
}
// end of wiCMatDelCol()

/*============================================================================+
|  Function  : wiCMatTrans()                                                  |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute the transpose of x and save it to y                    |
|  Parameters: y, x                                                           |
+============================================================================*/
wiStatus wiCMatTrans(wiCMat *y, wiCMat *x)
{
    int i, j;

    if (x->row != y->col || x->col != y->row) 
        return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH);

    for (i=0; i<y->row; i++)
        for (j=0; j<y->col; j++)
            y->val[i][j] = x->val[j][i];

    return WI_SUCCESS;
}
// end of wiCMatTrans()

/*============================================================================+
|  Function  : wiCMatConjTrans()                                              |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute the conjugate transpose of x and save it to y          |
|  Parameters: x, y
+============================================================================*/
wiStatus wiCMatConjTrans(wiCMat *y, wiCMat *x)
{
    int i, j;

    if (x->row!=y->col || x->col!=y->row) 
        return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 

    for (i=0; i<y->row; i++)
    {
        for (j=0; j<y->col; j++)
        {
            y->val[i][j].Real =  x->val[j][i].Real;
            y->val[i][j].Imag = -x->val[j][i].Imag;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatConjTrans()

/*============================================================================+
|  Function  : wiCMatScale()                                                  |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute y = scalar*x                                           |
|  Parameters: x, y, scalar                                                   |
+============================================================================*/
wiStatus wiCMatScale(wiCMat *y, wiCMat *x, wiComplex scalar)
{
    int i, j;

    if (x->col!=y->col || x->row!=y->row) 
        return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 

    for (i=0; i<x->row; i++)
    {
        for (j=0; j<x->col; j++)
        {
            COMPLEX_MULTIPLY(y->val[i][j], x->val[i][j], scalar);
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatScale()

/*============================================================================+
|  Function  : wiCMatScaleSelf()                                              |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute x = scalar * x                                         |
|  Parameters: x, scalar                                                      | 
+============================================================================*/
wiStatus wiCMatScaleSelf(wiCMat *x, wiComplex scalar)
{
    int i, j;
    wiComplex a;

    for (i=0; i<x->row; i++)
    {
        for (j=0; j<x->col; j++)
        {
            a = x->val[i][j];
            COMPLEX_MULTIPLY(x->val[i][j], a, scalar);
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatScaleSelf()

/*============================================================================+
|  Function  : wiCMatScaleSelfReal()                                          |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute x = scalar * x                                         |
|  Parameters: x, scalar                                                      | 
+============================================================================*/
wiStatus wiCMatScaleSelfReal(wiCMat *x, wiReal scalar)
{
    int i, j;

    for (i=0; i<x->row; i++)
    {
        for (j=0; j<x->col; j++)
        {
            x->val[i][j].Real *= scalar;
            x->val[i][j].Imag *= scalar;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatScaleSelfReal()

/*============================================================================+
|  Function  : wiCMatAdd()                                                    |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute z = x + y                                              |
|  Parameters: z x, y                                                         |        
+============================================================================*/
wiStatus wiCMatAdd(wiCMat *z, wiCMat *x,  wiCMat *y)
{
    int i, j;

    if (x->row!=y->row || x->col!=y->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    if (x->row!=z->row || x->col!=z->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    
    for (i=0; i<x->row; i++)
    {
        for (j=0; j<x->col; j++)
        {
            z->val[i][j].Real = x->val[i][j].Real + y->val[i][j].Real;
            z->val[i][j].Imag = x->val[i][j].Imag + y->val[i][j].Imag;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatAdd()

/*============================================================================+
|  Function  : wiCMatSub()                                                    |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute z = x - y                                              |
|  Parameters: z x, y                                                         |        
+============================================================================*/
wiStatus wiCMatSub(wiCMat *z, wiCMat *x,  wiCMat *y)
{
    int i, j;

    if (x->row!=y->row || x->col!=y->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    if (x->row!=z->row || x->col!=z->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    
    for (i=0; i<x->row; i++)
    {
        for (j=0; j<x->col; j++)
        {
            z->val[i][j].Real = x->val[i][j].Real - y->val[i][j].Real;
            z->val[i][j].Imag = x->val[i][j].Imag - y->val[i][j].Imag;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatSub()

/*============================================================================+
|  Function  : wiCMatAddSelf()                                                |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute y = x + y                                              |
|  Parameters: x, y                                                           |
+============================================================================*/
wiStatus wiCMatAddSelf(wiCMat *y, wiCMat *x)
{
    int i, j;
    
    if (x->row!=y->row || x->col!=y->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 

    for (i=0; i<y->row; i++)
    {
        for (j=0; j<y->col; j++)
        {
            y->val[i][j].Real += x->val[i][j].Real;
            y->val[i][j].Imag += x->val[i][j].Imag;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatAddSelf()

/*============================================================================+
|  Function  : wiCMatScaleAdd()                                               |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute z = scalar_x*x + scalar_y*y                            |
|  Parameters: z x, y, scalar_x, scalar_y                                     |        
+============================================================================*/
wiStatus wiCMatScaleAdd(wiCMat *z, wiCMat *x,  wiCMat *y, wiComplex scalar_x, wiComplex scalar_y)
{
    int i, j;
    wiComplex temp1, temp2;

    if (x->row!=y->row || x->col!=y->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    if (x->row!=z->row || x->col!=z->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    
    for (i=0; i<x->row; i++)
    {
        for (j=0; j<x->col; j++)
        {
            COMPLEX_MULTIPLY(temp1, scalar_x, x->val[i][j]);
            COMPLEX_MULTIPLY(temp2, scalar_y, y->val[i][j]);
            z->val[i][j].Real = temp1.Real + temp2.Real;
            z->val[i][j].Imag = temp1.Imag + temp2.Imag;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatScaleAdd()

/*============================================================================+
|  Function  : wiCMatScaleAddSelf()                                           |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute y = scalar_x*x + y                                     |
|  Parameters: x, y, scalar_x                                                 |
+============================================================================*/
wiStatus wiCMatScaleAddSelf(wiCMat *y, wiCMat *x, wiComplex scalar_x)
{
    int i, j;
    wiComplex temp;
    
    if (x->row!=y->row || x->col!=y->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 

    for (i=0; i<y->row; i++)
    {
        for (j=0; j<y->col; j++)
        {
            COMPLEX_MULTIPLY(temp, scalar_x, x->val[i][j]);
            y->val[i][j].Real += temp.Real;
            y->val[i][j].Imag += temp.Imag;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatScaleAddSelf()

/*============================================================================+
|  Function  : wiCMatMul()                                                    |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute z = x*y                                                |
|  Parameters: z, y, x                                                        |
+============================================================================*/
wiStatus wiCMatMul(wiCMat *z, wiCMat *x,  wiCMat *y)
{
    int i, j, k;

    // check size mismatch
    if (x->row!=z->row || y->col!=z->col || x->col!=y->row) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 

    // compute multiplication
    XSTATUS( wiCMatSetZero(z) );
    for (i=0; i<z->row; i++)
    {
        for (j=0; j<z->col; j++)
        {
            for (k=0; k<x->col; k++)
            {
                z->val[i][j].Real += ( (x->val[i][k].Real*y->val[k][j].Real) - (x->val[i][k].Imag*y->val[k][j].Imag) );
                z->val[i][j].Imag += ( (x->val[i][k].Real*y->val[k][j].Imag) + (x->val[i][k].Imag*y->val[k][j].Real) );
            }
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatMul()

/*============================================================================+
|  Function  : wiCMatMulSelfLeft()                                            |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute y=x*y                                                  |
|  Parameters: y,x                                                            |
+============================================================================*/
wiStatus wiCMatMulSelfLeft(wiCMat *y, wiCMat *x)
{
    wiCMat z;

    if (x->col!=y->row || x->row!=x->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 

    XSTATUS( wiCMatInit(&z, y->row, y->col) );
    XSTATUS( wiCMatMul (&z, x, y) );
    XSTATUS( wiCMatCopy(&z, y) );
    XSTATUS( wiCMatFree(&z) );

    return WI_SUCCESS;
}
// end of wiCMatMulSelfLeft() 

/*============================================================================+
|  Function  : wiCMatMulSelfRight()                                           |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute y=y*x                                                  |
|  Parameters: y,x                                                            |
+============================================================================*/
wiStatus wiCMatMulSelfRight(wiCMat *y, wiCMat *x)
{
    wiCMat z;

    if (x->row!=y->col || x->row!=x->col) return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 

    XSTATUS( wiCMatInit(&z, y->row, y->col) );
    XSTATUS( wiCMatMul (&z, y, x) );
    XSTATUS( wiCMatCopy(&z, y) );
    XSTATUS( wiCMatFree(&z) );

    return WI_SUCCESS;
}
// end of wiCMatMulSelfRight() 

/*============================================================================+
|  Function  : wiCMatFrobeNormSquared()                                       |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute the Frobenius Norm z = sqrt(sum of squares of all      |
|              elements);for vector, it is just the usual second-order norm   |
|  Parameters: x                                                              |
+============================================================================*/
wiReal wiCMatFrobeNormSquared(wiCMat *x)
{
    int i, j;
    wiReal Sx2 = 0.0;

    for (i=0; i<x->row; i++)
        for (j=0; j<x->col; j++)
            Sx2 += wiComplexNormSquared(x->val[i][j]);

    return Sx2;
}
// end of wiCMatFrobeNormSquared()

/*============================================================================+
|  Function  : wiCMatFrobeNorm()                                              |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute the Frobenius Norm z = sqrt(sum of squares of all      |
|              elements);for vector, it is just the usual second-order norm   |
|  Parameters: x                                                              |
+============================================================================*/
wiReal wiCMatFrobeNorm(wiCMat *x)
{
    return sqrt( wiCMatFrobeNormSquared(x) );
}
// end of wiCMatFrobeNorm()

/*============================================================================+
|  Function  : wiCMatTrace()                                                  |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute the trace                                              |
|  Parameters: x                                                              |
+============================================================================*/
wiComplex wiCMatTrace(wiCMat *x)
{
    wiInt i;
    wiComplex trace = {0,0};

    for (i=0; i<x->row; i++)
    {
        trace.Real += x->val[i][i].Real;
        trace.Imag += x->val[i][i].Imag;
    }
    return trace;
}
// end of wiCMatTrace()

/*============================================================================+
|  Function  : wiCMatGetSubMtx()                                              |
|-----------------------------------------------------------------------------|
|  Purpose   : Get the submatrix                                              |
|  Parameters: x, s_sub, i1, j1, i2, j2                                       |
+============================================================================*/
wiStatus wiCMatGetSubMtx(wiCMat *x, wiCMat *x_sub, wiInt i1, wiInt j1, wiInt i2, wiInt j2)
{
    int i, j;
    
    // check index out of range
    if (i1<0 || i1>x->row-1 || i2<0 || i2>x->row-1 || j1<0 || j1>x->col-1 || j2<0 || j2>x->col-1 ||
        i1>i2 || j1>j2 || (i2-i1+1)!=x_sub->row || (j2-j1+1)!=x_sub->col)
    {
        return STATUS(WI_ERROR_MATRIX_INDEX_OUT_OF_RANGE); 
    }

    for (i=0; i<i2-i1+1; i++)
        for (j=0; j<j2-j1+1; j++)
            x_sub->val[i][j] = x->val[i+i1][j+j1];

    return WI_SUCCESS;
}
// end of wiCMatGetSubMtx()

/*============================================================================+
|  Function  : wiCMatSetSubMtx()                                              |
|-----------------------------------------------------------------------------|
|  Purpose   : Rewrite a submatrix of x with x_sub                            |
|  Parameters: x, s_sub, i1, j1, i2, j2                                       |
+============================================================================*/
wiStatus wiCMatSetSubMtx(wiCMat *x, wiCMat *x_sub, wiInt i1, wiInt j1, wiInt i2, wiInt j2)
{
    int i, j;
    
    // check index out of range
    if (i1<0 || i1>x->row-1 || i2<0 || i2>x->row-1 || j1<0 || j1>x->col-1 || j2<0 || j2>x->col-1 ||
        i1>i2 || j1>j2 || (i2-i1+1)!=x_sub->row || (j2-j1+1)!=x_sub->col)
    {
        return STATUS(WI_ERROR_MATRIX_INDEX_OUT_OF_RANGE); 
    }
    for (i=0; i<i2-i1+1; i++)
        for (j=0; j<j2-j1+1; j++)
            x->val[i+i1][j+j1] = x_sub->val[i][j];

    return WI_SUCCESS;
}
// end of wiCMatSetSubMtx()

/*============================================================================+
|  Function  : wiCMatSubCorner()                                              |
|-----------------------------------------------------------------------------|
|  Purpose   : Substract from y a smaller x alighend at the right bottom,     |
|  Parameters: y,x                                                            |
+============================================================================*/
wiStatus wiCMatSubCorner(wiCMat *y, wiCMat *x)
{
    int i, j;
 
    // check size mismatch
    if (x->row > y->row || x->col > y->col) return STATUS(WI_ERROR_RANGE); 

    for (i=0; i<x->row; i++)
    {
        for (j=0; j<x->col; j++)
        {
            y->val[y->row-x->row+i][y->col-x->col+j].Real -= x->val[i][j].Real;
            y->val[y->row-x->row+i][y->col-x->col+j].Imag -= x->val[i][j].Imag;
        }  
    }
    return WI_SUCCESS;
}
// end of wiCMatSubCorner()

// ================================================================================================
// FUNCTION  : wiCMatSwapColumn()
// Purpose   : Swap two columns of a matrix;    
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiCMatSwapColumn(wiCMat *x, wiInt i, wiInt j)
{
	wiCMat y,z;

	if(i<0 || i>=x->col || j<0 || j>=x->col)
		return STATUS(WI_ERROR_MATRIX_INDEX_OUT_OF_RANGE); 	

	XSTATUS(wiCMatInit(&y, x->row, 1));
	XSTATUS(wiCMatInit(&z, x->row, 1));

	XSTATUS(wiCMatGetSubMtx(x, &y, 0, i, x->row-1, i));
	XSTATUS(wiCMatGetSubMtx(x, &z, 0, j, x->row-1, j));

	XSTATUS(wiCMatSetSubMtx(x, &y, 0, j, x->row-1, j));
	XSTATUS(wiCMatSetSubMtx(x, &z, 0, i, x->row-1, i));

	wiCMatFree(&y);
	wiCMatFree(&z);
	
	return WI_SUCCESS;
}

// =========================================================================================
// Function  : wiCMatComplexToReal()                                          
// Purpose   : Get the equivalent real matrix A1 of a complex matrix                      
// Parameters: A, A1, Block, RealOnly                                                         
// Note:       Assumes A1 has been initialized with zero imag  
//             DoubleColumn: 
//             0: size of A! is [2*A->row, A->col] row expantion of [Re, Im, ...., Re, Im]
//             1: size of A1 is [2*A->row, 2*A->col] with component expantion [Re -Im; Im Re] 
// =========================================================================================
wiStatus wiCMatComplexToReal(wiCMat *A, wiCMat *A1, wiInt DoubleColumn)
{
	wiInt i, j, k;

	if((A1->row != 2*A->row)) 			return STATUS(WI_ERROR_PARAMETER1);
	if(!DoubleColumn && (A1->col != A->col)) return STATUS(WI_ERROR_PARAMETER1); 
	
 	for (i=0; i<A->row; i++)
		for(j=0, k=0; j<A->col; j++, k++)
		{			
			A1->val[2*i][k].Real = A->val[i][j].Real;
			A1->val[2*i+1][k].Real = A->val[i][j].Imag;
		
			if(DoubleColumn)
			{
				A1->val[2*i][++k].Real = -A->val[i][j].Imag;
				A1->val[2*i+1][k].Real = A->val[i][j].Real;					
			}	
		}
	return WI_SUCCESS;
} // wiCMatComplexToReal

/*==============================================================================+
|  Function  : wiCMatQR()                                                       |
|-------------------------------------------------------------------------------|
|  Purpose   : Compute the QR decomposition of A = QR, by means of Householder  |
|              reflections; check website:                                      |
|                ***  http://en.wikipedia.org/wiki/QR_decomposition ***         |
|  Parameters: A, Q, R                                                          |   
|  Note:       It demands that row size of A is no less than column size;       |  
|              Suppose A is m-by-n, m>=n, then Q is m-by-n, R is n-by-n         |                                              
+==============================================================================*/
wiStatus wiCMatQR(wiCMat *A, wiCMat *Q,  wiCMat *R)
{
    wiInt i,col;
    wiReal a,b;
    wiComplex c;
    wiCMat x, x_h, xxh, A1,  Q1,  H;
    
    // initialize matrix memory
    XSTATUS( wiCMatInit(&A1,  A->row, A->col) );
    XSTATUS( wiCMatInit(&Q1,  A->row, A->row) );
    XSTATUS( wiCMatInit(&H,   A->row, A->row) );
    XSTATUS( wiCMatInit(&x,   A->row, 1     ) );
    XSTATUS( wiCMatInit(&x_h, 1,      A->row) );
    XSTATUS( wiCMatInit(&xxh, A->row, A->row) );

    XSTATUS( wiCMatCopy(A, &A1) );
    XSTATUS( wiCMatSetIden(&Q1) );

    for (col=0; col<A1.col; col++)
    {
        XSTATUS( wiCMatGetSubMtx(&A1, &x, col, col, A1.row-1, col) );

        a = wiCMatFrobeNorm(&x);
        b = wiComplexNorm(x.val[0][0]); 
        if (a>epsilon && fabs(a-b)>epsilon) // check whether x is a zero vector                                
        {                                   // and check whether x is a zero vector except the first element
            if (b>epsilon) // check whether the first element of x is zero
            {
                c.Real = x.val[0][0].Real/b;
                c.Imag = x.val[0][0].Imag/b;
            }
            else
            {
                c.Real = 1;
                c.Imag = 0;
            }
            // block above is used to avoid loss of significance;

            // compute Househoulder transform matrix
            x.val[0][0].Real -= (a * c.Real);
            x.val[0][0].Imag -= (a * c.Imag);
            a = wiCMatFrobeNorm(&x);
            b = sqrt(2.0)/a;
            for (i=0; i<x.row; i++)
            {
                x.val[i][0].Real *= b;
                x.val[i][0].Imag *= b;
            }
            XSTATUS( wiCMatConjTrans(&x_h, &x) );       
            XSTATUS( wiCMatMul(&xxh, &x, &x_h) );
            XSTATUS( wiCMatSetIden(&H) );
            XSTATUS( wiCMatSubCorner(&H,&xxh) );
        }
        else // if x is a zero vector, no transform is applied
        {
            XSTATUS( wiCMatSetIden(&H) );
        }
        XSTATUS( wiCMatMulSelfLeft (&A1, &H) ); // update on righ matrix
        XSTATUS( wiCMatMulSelfRight(&Q1, &H) ); // update on left unitary matrix

        XSTATUS( wiCMatSetZero(&x) );
        x.row -= 1;
        XSTATUS( wiCMatSetZero(&x_h) );
        x_h.col -= 1;
        XSTATUS( wiCMatSetZero(&xxh) );
        xxh.row -= 1; 
        xxh.col -= 1;
    }
    XSTATUS( wiCMatGetSubMtx(&Q1, Q, 0, 0, A->row-1, A->col-1) );
    XSTATUS( wiCMatGetSubMtx(&A1, R, 0, 0, A->col-1, A->col-1) );
    
    // free matrix memory
    wiCMatFree(&A1);
    wiCMatFree(&Q1);
    wiCMatFree(&H);
    wiCMatFree(&x);
    wiCMatFree(&x_h);
    wiCMatFree(&xxh);

    return WI_SUCCESS;
}
// end of wiCMatQR()

// =============================================================================
// Function  : wiCMatModifiedHHQR(): support only up to 8 columns                                          
// Purpose   : QR of A according to Eli Fogel's modified householder algorithm                             
// Parameters: A, Q, R, P                                                          
// Note:       It demands A is the equivalent real matrix for a complex matrix 
//             Each 2x2 component of the matrix has givens rotation structure as
//             [Real -Imag; Imag Real] 
//             The corresponding signal mapping is [Real Imag]
//             P: Permutation list
//             Sorted: 0: natural order 1: Ascending ordering
// ==============================================================================
wiStatus wiCMatModifiedHHQR(wiCMat *A, wiCMat *Q,  wiCMat *R, wiInt *P, wiInt Sorted)
{
    wiInt i, j, k, temp;
    wiReal sigma, lambda;
    wiComplex c;
    wiCMat A1, Q1, H, Ht, u1, u2, u11, u21, u1t, u2t,  Uu1t, Uu2t, G, Gt;
    wiReal NormA1[8], MinNormA1, a;
	if((A->row & 0x1) || (A->col & 0x1)) return STATUS(WI_ERROR_PARAMETER1);


    // initialize matrix memory
    XSTATUS( wiCMatInit(&A1,   A->row, A->col) );
    XSTATUS( wiCMatInit(&Q1,   A->row, A->row) );
    XSTATUS( wiCMatInit(&H,    A->row, A->row) );
	XSTATUS( wiCMatInit(&Ht,    A->row, A->row) );
   	XSTATUS( wiCMatInit(&u1,   A->row,      1) );
	XSTATUS( wiCMatInit(&u2,   A->row,      1) );
	XSTATUS( wiCMatInit(&u11,  A->row,      1) );
	XSTATUS( wiCMatInit(&u21,  A->row,      1) );
	XSTATUS( wiCMatInit(&u1t,  1,      A->row) );
	XSTATUS( wiCMatInit(&u2t,  1,      A->row) );	
    XSTATUS( wiCMatInit(&Uu1t, A->row, A->row) );
	XSTATUS( wiCMatInit(&Uu2t, A->row, A->row) );
	XSTATUS( wiCMatInit(&G,    2,           2) ); 
	XSTATUS( wiCMatInit(&Gt,   2,           2) ); 
	
    XSTATUS( wiCMatCopy(A, &A1) );
    XSTATUS( wiCMatSetIden(&Q1) );
	
	// initialize P as natual order
	for (j=0; j<A1.col; j++)
			P[j] = j;
				
    for (j=0; j<A1.col; j+=2)
    {
		// Sorting in ascending order
		if(Sorted) 
		{
			for (k=j; k<A1.col; k+=2)
			{
				XSTATUS( wiCMatGetSubMtx(&A1, &u1, j, k, A1.row-1, k) );
				// power of partial column vector norm
				NormA1[k] = wiCMatFrobeNormSquared(&u1);
				NormA1[k+1] = NormA1[k];
			}

			MinNormA1 = NormA1[j];
			i = j;

			for (k=j+2; k<A1.col; k+=2)
			{
				if(NormA1[k]<MinNormA1)
				{
					MinNormA1 = NormA1[k];
					i = k;
				}
			}

			// Swap column pairs and update permutation list and Norm correspondingly
			if(i!=j)
			{
				XSTATUS( wiCMatSwapColumn(&A1,i,j) );
				XSTATUS( wiCMatSwapColumn(&A1,i+1,j+1) );

				temp = P[j];
				P[j] = P[i];
				P[i] = temp;
								
				P[j+1] = P[j]+1;
				P[i+1] = P[i]+1;

				NormA1[i] = NormA1[j];
				NormA1[i+1] = NormA1[i];
				NormA1[j] = MinNormA1;
				NormA1[j+1] = NormA1[j];				
			}
		}

		// Get the first partial column of the pair, get its norm sigma
		XSTATUS( wiCMatGetSubMtx(&A1, &u1, j, j, A1.row-1, j) ); 
		sigma = wiCMatFrobeNorm(&u1);

		// Norm of the first complex component
		c.Real = u1.val[0][0].Real;
		c.Imag = u1.val[1][0].Real;
		lambda = wiComplexNorm(c);
		
		if (sigma > epsilon) // check whether x is a zero vector                                
        { 
			// rotation matrix
			G.val[0][0].Real = u1.val[0][0].Real;
			G.val[1][0].Real = u1.val[1][0].Real;
			G.val[0][1].Real = -u1.val[1][0].Real;
			G.val[1][1].Real = u1.val[0][0].Real;

			XSTATUS(wiCMatScaleSelfReal(&G, 1/lambda) );
			XSTATUS(wiCMatTrans(&Gt, &G) );

			u1.val[0][0].Real -= sigma*G.val[0][0].Real;
			u1.val[1][0].Real -= sigma*G.val[1][0].Real;
		
			XSTATUS( wiCMatGetSubMtx(&A1, &u2, j, j+1, A1.row-1, j+1) );
			u2.val[0][0].Real -= sigma*G.val[0][1].Real;
			u2.val[1][0].Real -= sigma*G.val[1][1].Real;

			// Modified reflection vectors
			XSTATUS(wiCMatCopy(&u1, &u11) );
			u11.val[0][0].Real = lambda-sigma;
			u11.val[1][0].Real = 0.0;
		
			XSTATUS(wiCMatCopy(&u2, &u21) );
			u21.val[0][0].Real = 0.0;
			u21.val[1][0].Real = lambda-sigma;
		
			XSTATUS(wiCMatTrans(&u1t, &u1) );
			XSTATUS(wiCMatTrans(&u2t, &u2) );
			
			a = wiCMatFrobeNormSquared(&u1);
			if(a > epsilon)
			{
				XSTATUS(wiCMatMul(&Uu1t, &u11,  &u1t) );
				XSTATUS(wiCMatScaleSelfReal(&Uu1t, 2/a) );
			}

			a = wiCMatFrobeNormSquared(&u2);			
			if(a > epsilon)
			{
				XSTATUS(wiCMatMul(&Uu2t, &u21,  &u2t) );
				XSTATUS(wiCMatScaleSelfReal(&Uu2t, 2/a) );
			}

			XSTATUS( wiCMatSetIden(&H) );
			
			// Set the rotation matrix
			XSTATUS( wiCMatSetSubMtx(&H, &Gt, j, j, j+1, j+1) ); 
			// Get the modified Householder matrix
			XSTATUS( wiCMatSubCorner(&H,&Uu1t) );
			XSTATUS( wiCMatSubCorner(&H,&Uu2t) );

		}
		else		
			XSTATUS( wiCMatSetIden(&H) );

		XSTATUS(wiCMatTrans(&Ht, &H) );

		XSTATUS( wiCMatMulSelfLeft (&A1, &H) ); // update on righ matrix
        XSTATUS( wiCMatMulSelfRight(&Q1, &Ht) ); // update on left unitary matrix

		XSTATUS( wiCMatSetZero(&u1) );
		u1.row -= 2;

		XSTATUS( wiCMatSetZero(&u2) );
		u2.row -= 2;

		XSTATUS( wiCMatSetZero(&u11) );
		u11.row -= 2;

		XSTATUS( wiCMatSetZero(&u21) );
		u21.row -= 2;

		XSTATUS( wiCMatSetZero(&u1t) );
		u1t.col -= 2;

		XSTATUS( wiCMatSetZero(&u2t) );
		u2t.col -= 2;

		XSTATUS( wiCMatSetZero(&Uu1t) );
        Uu1t.row -= 2; 
        Uu1t.col -= 2;

		XSTATUS( wiCMatSetZero(&Uu2t) );
        Uu2t.row -= 2; 
        Uu2t.col -= 2;
	}

	XSTATUS( wiCMatGetSubMtx(&Q1, Q, 0, 0, A->row-1, A->col-1) );
    XSTATUS( wiCMatGetSubMtx(&A1, R, 0, 0, A->col-1, A->col-1) );

    // free matrix memory
    wiCMatFree(&A1);
    wiCMatFree(&Q1);
    wiCMatFree(&H);
	wiCMatFree(&Ht);
    wiCMatFree(&u1);
    wiCMatFree(&u11);
	wiCMatFree(&u1t);
    wiCMatFree(&Uu1t);
	wiCMatFree(&u2);
    wiCMatFree(&u21);
	wiCMatFree(&u2t);
    wiCMatFree(&Uu2t);
	wiCMatFree(&G);
	wiCMatFree(&Gt);

    return WI_SUCCESS;
}
// end of wiCMatModifiedHHQR()

// ================================================================================================
// FUNCTION  : wiCMatAscendingSortQR()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the sorted QR decomposition of A = QR, by means of Gram-Schmidt method;
//             It intends (but not guarantees) to sort diangonal elements of R in ascending order
//             Based on Wubben's Efficient algorithm for decoding LSTC 2002
// Parameters: A, Q, R, p
// Note      : p is the permutation vector            
// ================================================================================================
wiStatus wiCMatAscendingSortQR(wiCMat *A, wiCMat *Q,  wiCMat *R, wiInt *P)
{
    wiInt i,col, ki, j;
    wiComplex c;
    wiCMat x, y;
    wiReal *NormQ, MinNormQ;  
	
	if(!P) return STATUS(WI_ERROR_PARAMETER4);
	
	XSTATUS(wiCMatInit(&x, A->row, 1));
	XSTATUS(wiCMatInit(&y, A->row, 1));	
	XSTATUS(wiCMatCopy(A, Q) );
    XSTATUS(wiCMatSetZero(R) );
	WICALLOC(NormQ, A->col, wiReal);
		
	// initialize P as natual order
	for (col=0; col<Q->col; col++)
	{
		P[col] = col;
		XSTATUS(wiCMatGetSubMtx(Q, &x, 0, col, Q->row-1, col));
		NormQ[col] = wiCMatFrobeNormSquared(&x);		
	}
		
    for (col=0; col<Q->col; col++)
    {
        MinNormQ = NormQ[col];
		ki = col;
		for (j=col+1; j<Q->col; j++)
		{
			if(NormQ[j]<MinNormQ)
			{
				MinNormQ = NormQ[j];
				ki = j;
			}
		}
		
		// Swap columns
		if(ki!= col)
		{
			XSTATUS(wiCMatSwapColumn(Q,ki,col));
			XSTATUS(wiCMatSwapColumn(R,ki,col));

			i = P[col];
			P[col] = P[ki];
			P[ki]  = i;

			NormQ[ki]  = NormQ[col];
			NormQ[col] = MinNormQ;
		}

		R->val[col][col].Real = sqrt(NormQ[col]);
		R->val[col][col].Imag = 0.0;
		c.Real = 1.0/R->val[col][col].Real; c.Imag = 0.0; 
		
		// Generate the col th base
		XSTATUS(wiCMatGetSubMtx(Q, &x, 0, col, Q->row-1, col));
		XSTATUS(wiCMatScaleSelf(&x, c));
		XSTATUS(wiCMatSetSubMtx(Q, &x, 0, col, Q->row-1, col));
		
		// Remove the component from base col from jth column of Q, update Norm
		for (j=col+1; j<Q->col; j++)
		{			
			XSTATUS(wiCMatGetSubMtx(Q, &y, 0, j, Q->row-1, j));

			// Inner product of two vector
			for (i=0; i<x.row; i++)
			{
				R->val[col][j].Real += x.val[i][0].Real*y.val[i][0].Real + x.val[i][0].Imag*y.val[0][i].Imag;
				R->val[col][j].Imag += x.val[i][0].Real*y.val[i][0].Imag - x.val[i][0].Imag*y.val[0][i].Real; 
			}
			
			c.Real = -1.0*R->val[col][j].Real; c.Imag = -1.0*R->val[col][j].Imag;
			XSTATUS(wiCMatScaleAddSelf(&y, &x, c)); 

			NormQ[j] -= pow(c.Real,2.0) + pow(c.Imag,2.0);
	
			XSTATUS(wiCMatSetSubMtx(Q, &y, 0, j, Q->row-1, j));			
		}
	}

    // free matrix memory
    wiCMatFree(&x);
    wiCMatFree(&y);
    wiFree(NormQ);

    return WI_SUCCESS;
}
// end of wiCMatAscendingSortQR()

// ================================================================================================
// FUNCTION  : wiCMatQRWrapper()
// ------------------------------------------------------------------------------------------------
// Purpose   : QR function wrapper, calling different QR functions according to input option
// Parameters: A, Q, R, P
//             P: the permutation vector     
//             ModifiedQR: 0 column-wise QR  1: column-pair QR with Eli's modified householder QR
//
// ================================================================================================
wiStatus wiCMatQRWrapper(wiCMat *A, wiCMat *Q,  wiCMat *R, wiInt *P, wiInt ModifiedQR, wiInt Sorted )
{
    int i;
			
	if (ModifiedQR)
	{		
		XSTATUS(wiCMatModifiedHHQR(A, Q, R, P, Sorted));
	}
	else 
	{	
		if (Sorted)
		{			
			XSTATUS(wiCMatAscendingSortQR(A, Q, R, P));
		}
		else
		{			
			// Initialize P in natural order
			for (i=0; i<A->col; i++) P[i] = i;
			XSTATUS(wiCMatQR(A, Q, R));
		}
	}

    return WI_SUCCESS;
}
// end of wiCMatQRWrapper()


/*============================================================================+
|  Function  : wiCMatInv()                                                    |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute inverse of B= inv(A)  using QR decomposition           |
|  Parameters: B, A                                                           |
+============================================================================*/
wiStatus wiCMatInv(wiCMat *B, wiCMat *A)
{
    wiInt i,j,k;
    wiComplex a;
    wiCMat Q, Q_inv, R, R_inv;
    
    XSTATUS( wiCMatInit(&Q,     A->row, A->row) ); 
    XSTATUS( wiCMatInit(&Q_inv, A->row, A->row) ); 
    XSTATUS( wiCMatInit(&R,     A->row, A->row) ); 
    XSTATUS( wiCMatInit(&R_inv, A->row, A->col) ); 

    XSTATUS( wiCMatQR(A, &Q,  &R) );        // compute QR decomposition
    XSTATUS( wiCMatConjTrans(&Q_inv, &Q) ); // compute the inverse of Q, which is unitary
    
    // compute the inverse of R using the fact that the inverse of an upper 
    // triangular matrix is also upper-triangular
    for (i=0; i<A->row; i++)
    {
//        wiStatus status = wiComplexInv(&(R_inv.val[i][i]), &(R.val[i][i]));
        if (R.val[i][i].Real == 0 && R.val[i][i].Imag == 0)
        {
            wiPrintf("hi\n");
            wiPrintf("A = \n"); wiCMatDisplay(A);
            wiPrintf("R = \n"); wiCMatDisplay(&R);
            XSTATUS(WI_ERROR_DIVIDE_BY_ZERO);
        }
        R_inv.val[i][i] = wiComplexInv(R.val[i][i]);
        for (j=i+1; j<A->row; j++)
        {
            R_inv.val[i][j].Real = 0;
            R_inv.val[i][j].Imag = 0;
            for (k=i; k<=j-1; k++)
            {
                a = wiComplexMul(R_inv.val[i][k], R.val[k][j]);
                R_inv.val[i][j].Real += a.Real;
                R_inv.val[i][j].Imag += a.Imag;
            }
            a.Real = -R_inv.val[i][j].Real;
            a.Imag = -R_inv.val[i][j].Imag;
            R_inv.val[i][j] = wiComplexDiv(a, R.val[j][j]);
        }
    }
    XSTATUS( wiCMatMul(B, &R_inv, &Q_inv) ); // compute the inverse

    wiCMatFree(&Q);
    wiCMatFree(&Q_inv);
    wiCMatFree(&R);
    wiCMatFree(&R_inv);
    
    return WI_SUCCESS;
}
// end of wiCMatInv()

/*============================================================================+
|  Function  : wiCMatDisplay()                                                |
|-----------------------------------------------------------------------------|
|  Purpose   : Display elements of x                                          |
|  Parameters: x                                                              |
+============================================================================*/
wiStatus wiCMatDisplay(wiCMat *x)
{
    int i, j;

    wiPrintf("\n---- matrix display ----\n");
    for (i=0; i<x->row; i++)
    {   
        for (j=0; j<x->col; j++)
        {
            wiPrintf("%.6f+i%.6f  ", x->val[i][j].Real, x->val[i][j].Imag);
        }  
        wiPrintf("\n");
    }
    wiPrintf("\n---- ---------- ----\n");
    return WI_SUCCESS;
}
// end of wiCMatDisplay()

/*============================================================================+
|  Function  : wiCMatKron()                                                   |
|-----------------------------------------------------------------------------|
|  Purpose   : Compute z = Kron(x,y); i.e. the Kronecker product of x and y   |
|  Parameters: z (mp-by-nq) , y (m-by-n) , x (p-by-q)                         |
+============================================================================*/
wiStatus wiCMatKron(wiCMat *z, wiCMat *x,  wiCMat *y)
{
    wiInt i,j,k,r,s,t;
    wiComplex temp;

    // check size mismatch
    if (x->row * y->row != z->row || x->col * y->col !=z->col)
    {
        return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    }
    // Kronecker multiplication
    for (i=0; i < x->row; i++)
        for (j=0; j < x->col; j++)
            for (k=0; k < y->row; k++)
                for (r=0; r < y->col; r++)
                {
                    COMPLEX_MULTIPLY(temp, x->val[i][j], y->val[k][r]);
                    s = i * y->row + k;
                    t = j * y->col + r;
                    z->val[s][t].Real = temp.Real;
                    z->val[s][t].Imag = temp.Imag;
                }

    return WI_SUCCESS;
}
// end of wiCMatKron()

/*============================================================================+
|  Function  : wiCMatCholesky()                                               |
|-----------------------------------------------------------------------------|
|  Purpose   : Cholesky Decomposition, i.e. R = L* L                          |
|  Parameters: L and R (row-by-col)                                           |
+============================================================================*/
wiStatus wiCMatCholesky(wiCMat *L, wiCMat *R)
{
    wiCMat Aold, Anew, Li;
    double ai, sqrt_ai;
    int n=R->row, i,j,k;
    wiComplex c;

    // check size mismatch
    if (L->row != R->row || L->col != R->col)
    {
        return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH); 
    }

    XSTATUS( wiCMatInit(&Aold, n, n) );
    XSTATUS( wiCMatInit(&Anew, n, n) );
    XSTATUS( wiCMatInit(&Li, n, n) );
    XSTATUS( wiCMatSetIden(L) );

    XSTATUS( wiCMatCopy(R, &Aold) );

    for (i=0; i<n; i++)
    {
        //update Li
        XSTATUS( wiCMatSetIden(&Li) );
        ai = Aold.val[i][i].Real;
        Li.val[i][i].Real = sqrt_ai = sqrt(Aold.val[i][i].Real);   

        if (i<n-1)
        {
            //   update Li
            for (j=i+1; j<n; j++)
            {
                Li.val[i][j].Real =  (Aold.val[j][i].Real) / sqrt_ai;
                Li.val[i][j].Imag = -(Aold.val[j][i].Imag) / sqrt_ai;
            }

            //   update Anew
            XSTATUS( wiCMatSetIden(&Anew) );
            for (j=i+1; j<n; j++)
                for (k=i+1; k<n; k++)
                {
                    c.Real = (Aold.val[j][i].Real * Aold.val[i][k].Real) - (Aold.val[j][i].Imag * Aold.val[i][k].Imag);
                    c.Imag = (Aold.val[j][i].Real * Aold.val[i][k].Imag) + (Aold.val[j][i].Imag * Aold.val[i][k].Real);

                    Anew.val[j][k].Real = Aold.val[j][k].Real - c.Real/ai;
                    Anew.val[j][k].Imag = Aold.val[j][k].Imag - c.Imag/ai;
                }
            XSTATUS( wiCMatCopy(&Anew, &Aold) );
        }
        XSTATUS( wiCMatMulSelfLeft(L, &Li) );
    }

    wiCMatFree(&Aold);
    wiCMatFree(&Anew);
    wiCMatFree(&Li);
    return WI_SUCCESS;
}
// end of wiCMatCholesky()


/*============================================================================+
|  Function  : wiCMatMul3()                                                   |
|-----------------------------------------------------------------------------|
|  Purpose   : Triple matrix multiplication:  A = U * S * V*                  |
|  Parameters: A (m-by-n, not initialized), U (m-by-j), S (j-by-k), V (k-by-n)|
+============================================================================*/
wiStatus wiCMatMul3(wiCMat *A, wiCMat *U,  wiCMat *S,  wiCMat *V) // A = U * S * V*
{
    wiCMat Vt, B;
    if (A) wiCMatFree(A);
    if ( (U->col != S->row) || (S->col != V->col) ) 
        return STATUS(WI_ERROR_MATRIX_SIZE_MISMATCH);

    XSTATUS( wiCMatInit(&Vt, V->col, V->row) );
    XSTATUS( wiCMatInit(&B, S->row, Vt.col) );
    XSTATUS( wiCMatInit(A, U->row, B.col) );

    XSTATUS( wiCMatConjTrans(&Vt, V) ); // Vt = V*
    XSTATUS( wiCMatMul(&B, S, &Vt) );// B = S * Vt
    XSTATUS( wiCMatMul(A, U, &B) );// A = U * B
    
    wiCMatFree(&Vt);
    wiCMatFree(&B);
    return WI_SUCCESS;
}
// End of wiCMatMul3()

/*============================================================================+
|  Function  : wiCMatROT()                                                    |
|-----------------------------------------------------------------------------|
|  Purpose   : This is an auxiliary function in the Demmel-Kahan SVD algorithm|
|  Parameters:                                                                |
+============================================================================*/
wiStatus wiCMatROT(double *cs, double *sn, double *rs, double f, double g)
{
    double t, tt;

    if (f==0)
    {
        *cs=0; *sn=1; *rs=g;
    }
    else
    {
        if (fabs(f)>fabs(g))
        {
            t=g/f;
            tt=sqrt(1+SQR(t));
            *cs=1/tt; *sn=t * *cs; *rs=f*tt;
        }
        else
        {
            t=f/g;
            tt=sqrt(1+SQR(t));
            *sn=1/tt; *cs=t * *sn; *rs=g*tt;
        }
    }
    return WI_SUCCESS;
}
// End of wiCMatROT()


/*============================================================================+
|  Function  : wiCMatSVD()                                                    |
|-----------------------------------------------------------------------------|
|  Purpose   : Complex Singular Value Decomposition, i.e. A = U S V*          |
|               Bidiagonalization based on the householder transforms,        |
|               and the SVD based on the Demmel-Kahan algorithm:              |
|                  "Accurate Singular Values of Bidiagonal Matrices",         |
|                  SIAM J. Sci. Stat. Comput., v. 11, n. 5, pp. 873-912, 1990.|
|  Parameters: A is the m-by-n input matrix                                   |
|            U (m-by-m) and V (n-by-n) are unitary matrices of sigular vectors|
|            S (m-by-n) is a diagonal matrix containing the singular values   |
|The sigular values are real, positive, and decreasingly sorted by magnitude. |
+============================================================================*/
wiStatus wiCMatSVD(wiCMat *U, wiCMat *S, wiCMat *V, //Output matrices
                                        wiCMat *A) //Input matrix
{
    wiBoolean Show = 0;
    int row,col, p, swapflag;

    wiCMat AA, B, Bt,
          Q, P, Qi, Pi,
          R, L, Ri, Li,
          Sign;
    int i, j, q, r, iter;
    double sum, nu, *s, *e, Error,
          cs, sn, rs, oldcs, oldsn=0, h;
    wiComplex *w, invsigma, temp;
    
    // Check the size of A
    if (Show) wiPrintf("Check the size of A\n");
    if (A->col > A->row) // We transpose A to get row>col
    {
        row = A->col;
        col = A->row;
        XSTATUS( wiCMatInit(&AA, row, col) );
        XSTATUS( wiCMatConjTrans(&AA, A) );      // AA=A*
        swapflag = 1;
    }
    else
    {
        row = A->row;
        col = A->col;
        XSTATUS( wiCMatInit(&AA, row, col) ); 
        XSTATUS( wiCMatCopy(A, &AA) );         // AA=A
        swapflag = 0;
    }
    
    XSTATUS( wiCMatInit(&B,  row, col) );
    XSTATUS( wiCMatInit(&Bt, col, row) );  //Bt = B*
    p = min(row,col);
    
    // ***********************************
    // * STEP 1: Bidiagonalization of AA *
    // ***********************************
    if (Show) wiPrintf("STEP 1: Bidiagonalization of AA\n");
    
    // Initialization
    XSTATUS( wiCMatCopy(&AA, &B) );
    XSTATUS( wiCMatInit(&Q, row, row) );
    XSTATUS( wiCMatSetIden(&Q) );
    XSTATUS( wiCMatInit(&P, col, col) );
    XSTATUS( wiCMatSetIden(&P) );
    // Temporary matrices, they accumulate in Q and P
    XSTATUS( wiCMatInit(&Qi, row, row) );  
    XSTATUS( wiCMatInit(&Pi, col, col) );    
    
    w = (wiComplex *)wiCalloc(max(row, col), sizeof(wiComplex));
    if (!w) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    
    for (i=0; i<p; i++)
    {
        // WORK ON COLUMNS
        // x=B(i:m,i);  nu=norm(x)*sign(real(x(1)));
        sum=0;
        for (j=i; j<row; j++)
            sum += SQR(B.val[j][i].Real) + SQR(B.val[j][i].Imag);
        nu = sqrt(sum);
        if (B.val[i][i].Real <0) nu=-nu;
        
        // w = x + nu * [1 0 ... 0]T
        // sigma = 1/(x* w)
        invsigma.Real=0;
        invsigma.Imag=0;
        for (j=i; j<row; j++)
        {
            w[j-i].Real = B.val[j][i].Real;
            w[j-i].Imag = B.val[j][i].Imag;
            if (j==i) w[j-i].Real += nu;
            
            // invsigma = x w*;
            invsigma.Real += B.val[j][i].Real * w[j-i].Real;
            invsigma.Real += B.val[j][i].Imag * w[j-i].Imag;
            invsigma.Imag -= B.val[j][i].Real * w[j-i].Imag;
            invsigma.Imag += B.val[j][i].Imag * w[j-i].Real;
        }
        
        XSTATUS( wiCMatSetIden(&Qi) );
        for (q=i; q<row; q++)
        {
            for (r=i; r<row; r++)
            {
                // temp = -w[q-i] w[r-i]*
                temp.Real = -w[q-i].Real * w[r-i].Real;
                temp.Real -= w[q-i].Imag * w[r-i].Imag;
                temp.Imag =  w[q-i].Real * w[r-i].Imag;
                temp.Imag -= w[q-i].Imag * w[r-i].Real;
                Qi.val[q][r] = wiComplexDiv(temp, invsigma);
                
                if (q==r)  // Diagonal temrs
                    Qi.val[q][r].Real += 1;
            }
        }
        
        XSTATUS( wiCMatMulSelfLeft(&Q, &Qi) );// Q = Qi * Q
        XSTATUS( wiCMatMul3(&B, &Q, &AA, &P) );
        XSTATUS( wiCMatConjTrans(&Bt, &B) );
                
        
        // WORK ON ROWS
        if (i < col-1)
        {
            // x=Bt(i+1:n,i);  nu=norm(x)*sign(real(x(1)));
            sum=0;
            for (j=i+1; j<col; j++)
                sum += SQR(Bt.val[j][i].Real) + SQR(Bt.val[j][i].Imag);
            nu = sqrt(sum);
            if (Bt.val[i][i].Real <0) nu=-nu;
            
            // w = x + nu * [1 0 ... 0]T
            // sigma = 1/(x* w)
            invsigma.Real=0;
            invsigma.Imag=0;
            for (j=i+1; j<col; j++)
            {
                w[j-i-1].Real = Bt.val[j][i].Real;
                w[j-i-1].Imag = Bt.val[j][i].Imag;
                if (j==i+1) w[j-i-1].Real += nu;
                
                // invsigma = x w*;
                invsigma.Real += Bt.val[j][i].Real * w[j-i-1].Real;
                invsigma.Real += Bt.val[j][i].Imag * w[j-i-1].Imag;
                invsigma.Imag -= Bt.val[j][i].Real * w[j-i-1].Imag;
                invsigma.Imag += Bt.val[j][i].Imag * w[j-i-1].Real;
            }
            
            XSTATUS( wiCMatSetIden(&Pi) );
            for (q=i+1; q<col; q++)
            {
                for (r=i+1; r<col; r++)
                {
                    // temp = -w[q-i-1] w[r-i-]*
                    temp.Real  =-w[q-i-1].Real * w[r-i-1].Real;
                    temp.Real -= w[q-i-1].Imag * w[r-i-1].Imag;
                    temp.Imag  = w[q-i-1].Real * w[r-i-1].Imag;
                    temp.Imag -= w[q-i-1].Imag * w[r-i-1].Real;
                    Pi.val[q][r] = wiComplexDiv(temp, invsigma);
                    
                    if (q==r)  // Diagonal terms
                        Pi.val[q][r].Real += 1;
                }
            }
            
            XSTATUS( wiCMatMulSelfLeft(&P, &Pi) );// P = Pi * P
            XSTATUS( wiCMatMul3(&B, &Q, &AA, &P) );
        }
    }
    
    // B is supposed to be bidiagonal, and have only real entries:
    for (i=0; i<row; i++)
    {
        for (j=0; j<col; j++)
        {
            B.val[i][j].Imag=0;
            if ( (j<i) || (j>i+1))
                B.val[i][j].Real=0;
        }
    }
    // End of bidiagonalization
    
    // ******************************************
    // * STEP 2: Iterative diagonalization of B *
    // ******************************************
    if (Show) wiPrintf("STEP 2: Iterative diagonalization of B\n");
    // Initialization
    wiCMatInit(&L, row, row);
    wiCMatSetIden(&L);
    wiCMatInit(&R, col, col);
    wiCMatSetIden(&R);
    // Temporary matrices, they accumulate in Q and P
    wiCMatInit(&Li, row, row);  
    wiCMatInit(&Ri, col, col);
    s = (double *) wiCalloc(p, sizeof(double));
    e = (double *) wiCalloc(p-1, sizeof(double));
    if ((!s) || (!e)) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    
    // s=diag(B,0) diagonal elements; e=diag(B,1) superdiagonal elements;   
    for (i=0; i<p; i++)
    {
        s[i]=B.val[i][i].Real;
        if (i<p-1) e[i]=B.val[i][i+1].Real;
    }
    
    // Main iterations
    Error=10.0;
    // [Barrett-110214] the previous line is below. I added the AND condition assuming
    // this was what Yingqun intended as it would make sense to continue a loop until the
    // error is below a threshold.
    //
    //for (iter=0; iter<3*p, Error>0.02; iter++)
    for (iter=0; (iter < 3*p) && (Error > 0.02); iter++)
    {
        oldcs=1;
        cs=1;
        for (i=0; i<p-1; i++)
        {
            wiCMatROT(&cs, &sn, &rs, s[i]*cs, e[i]);
            wiCMatSetIden(&Ri);
            Ri.val[i][i].Real = cs;
            Ri.val[i][i+1].Real = sn;
            Ri.val[i+1][i].Real = -sn;
            Ri.val[i+1][i+1].Real = cs;
            wiCMatMulSelfLeft(&R, &Ri); // R=Ri*R
            
            if (i!=0)
                e[i-1]=oldsn*rs;
            wiCMatROT(&oldcs, &oldsn, &(s[i]), oldcs*rs, s[i+1]*sn);
            wiCMatSetIden(&Li);
            Li.val[i][i].Real = oldcs;
            Li.val[i][i+1].Real = oldsn;
            Li.val[i+1][i].Real = -oldsn;
            Li.val[i+1][i+1].Real = oldcs;
            wiCMatMulSelfLeft(&L, &Li); // L=Li*L
        }
        h=s[p-1]*cs;
        e[p-2]=h*oldsn;
        s[p-1]=h*oldcs;
        
        Error=0.0;
        for (i=0; i<p-1; i++)
            if (fabs(e[i])>Error) Error=fabs(e[i]);
    }
    
    // Now produce the results
/*   if (!(U->val)) wiCMatInit(U, A->row, A->row);
    if (!S->val) wiCMatInit(S, A->row, A->col);
    if (!V->val) wiCMatInit(V, A->col, A->col);
*/   
    wiCMatMulSelfRight(&L, &Q); //L=LQ
    wiCMatMulSelfRight(&R, &P); //R=RP
    if (Show) wiPrintf("Check if the matrices need conjugation, depending on swapflag\n");
    if (swapflag==0)
    {
        wiCMatConjTrans(U, &L); // U= Q* L*
        wiCMatConjTrans(V, &R); // V= P* R*
    }
    else
    {
        wiCMatConjTrans(U, &R);
        wiCMatConjTrans(V, &L);
    }
    
    // Now, look for negative singular values, and make them positive.
    if (Show) wiPrintf("Look for negative singular values, and make them positive\n");
    wiCMatInit(&Sign, A->row, A->row); // This matrix gets multiplied by both S and U to make things positive
    for (i=0; i < A->row; i++)
    {
        if (i<p) 
        {
            if (s[i]<0)
            {
                
                Sign.val[i][i].Real = -1;
                s[i] = -s[i];
            }
            else
                Sign.val[i][i].Real = 1;
            
            S->val[i][i].Real = s[i];
        }
        else
            Sign.val[i][i].Real = 1;
        
    }
    wiCMatMulSelfRight(U, &Sign); // U = U Sign
    
    // Free the assigned memory
    if (Show) wiPrintf("Free the assigned memory\n");
    wiCMatFree(&AA); 
    wiCMatFree(&B); 
    wiCMatFree(&Bt); 
    wiCMatFree(&Q); 
    wiCMatFree(&P); 
    wiCMatFree(&Qi); 
    wiCMatFree(&Pi); 
    wiCMatFree(&R); 
    wiCMatFree(&L); 
    wiCMatFree(&Ri); 
    wiCMatFree(&Li); 
    wiCMatFree(&Sign);
    wiFree(w);

    if (Show) wiPrintf(" End of SVD\n");
    return WI_SUCCESS;
}
// End of wiCMatSVD

/*============================================================================+
|  Function  : wiCMatTestSVD()                                                |
|-----------------------------------------------------------------------------|
|  Purpose   : To test wiCMatSVD                                              |
|  Parameters:                                                                |
+============================================================================*/
int wiCMatTestSVD()
{
    wiCMat A, U, S, V;
    int row, col;
    
    wiCMatInit(&A, 4, 2);
    row=A.row;
    col=A.col;

    wiCMatInit(&U, row, row);
    wiCMatInit(&S, row, col);
    wiCMatInit(&V, col, col);
    
    A.val[0][0].Real =1;
    A.val[0][1].Real =2;
    A.val[1][0].Real =3;
    A.val[1][1].Real =4;
    A.val[2][0].Real =1.4;
    A.val[2][1].Real =-3.2;
    A.val[3][0].Real =-4.2;
    A.val[3][1].Real =-0.2;

    A.val[0][0].Imag =5;
    A.val[0][1].Imag =4;
    A.val[1][0].Imag =3;
    A.val[1][1].Imag =2;
    A.val[2][0].Imag =-3.4;
    A.val[2][1].Imag =7.1;
    A.val[3][0].Imag =-2;
    A.val[3][1].Imag =1.9;

    wiCMatSVD(&U, &S, &V, &A);
    
    wiPrintf("A = \n");    wiCMatDisplay(&A);
    wiPrintf("U = \n");    wiCMatDisplay(&U);
    wiPrintf("S = \n");    wiCMatDisplay(&S);
    wiPrintf("V = \n");    wiCMatDisplay(&V);
    {
        int key;
        scanf("%d",&key);
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

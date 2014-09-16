/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#include "gc_glff_precomp.h"

#define _GC_OBJ_ZONE glvZONE_MATRIX

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  glfUpdateMatrixStates
**
**  Update matrix states.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
**
*/

void glfUpdateMatrixStates(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    /* Make sure everything is computed and up to date. */
    glfGetModelViewInverse3x3TransposedMatrix(Context);
    glfGetModelViewProjectionMatrix(Context);

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _UpdateMatrixFlags
**
**  Updates special value flags of the specified matrix.
**
**  INPUT:
**
**      Matrix
**          Matrix to be updated.
**
**  OUTPUT:
**
**      Nothing.
*/

static gctBOOL _UpdateMatrixFlags(
    glsMATRIX_PTR Matrix
    )
{
    GLint x, y;

    gcmHEADER_ARG("Matrix=0x%x", Matrix);

    switch (Matrix->type)
    {
    case glvFIXED:
        Matrix->identity = GL_TRUE;

        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                if (x == y)
                {
                    if (glmMATFIXED(Matrix, x, y) != gcvONE_X)
                    {
                        Matrix->identity = GL_FALSE;
                        gcmFOOTER_NO();
                        return gcvTRUE;
                    }
                }
                else
                {
                    if (glmMATFIXED(Matrix, x, y) != gcvZERO_X)
                    {
                        Matrix->identity = GL_FALSE;
                        gcmFOOTER_NO();
                        return gcvTRUE;
                    }
                }
            }
        }
        break;

#if !defined(COMMON_LITE)
    case glvFLOAT:
        Matrix->identity = GL_TRUE;

        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                if (x == y)
                {
                    if (glmMATFLOAT(Matrix, x, y) != 1.0f)
                    {
                        Matrix->identity = GL_FALSE;
                        gcmFOOTER_NO();
                        return gcvTRUE;
                    }
                }
                else
                {
                    if (glmMATFLOAT(Matrix, x, y) != 0.0f)
                    {
                        Matrix->identity = GL_FALSE;
                        gcmFOOTER_NO();
                        return gcvTRUE;
                    }
                }
            }
        }
        break;
#endif

    default:
        gcmFATAL("_UpdateMatrixFlags: invalid type %d", Matrix->type);
        gcmFOOTER_NO();
        return gcvFALSE;
    }
    gcmFOOTER_NO();
    return gcvTRUE;
}


/*******************************************************************************
**
**  _LoadIdentityMatrix
**
**  Initializes the matrix to "identity".
**
**  INPUT:
**
**      Matrix
**          Pointer to the matrix to be reset.
**
**      Type
**          Type of the matrix to be produced.
**
**  OUTPUT:
**
**      Matrix
**          Identity matrix.
*/

static void _LoadIdentityMatrix(
    glsMATRIX_PTR Matrix,
    gleTYPE Type
    )
{
    GLint x, y;

    gcmHEADER_ARG("Matrix=0x%x Type=0x%04x", Matrix, Type);

    switch (Type)
    {
    case glvFIXED:
        Matrix->type = glvFIXED;

        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                glmMATFIXED(Matrix, x, y) = (x == y)
                    ? gcvONE_X
                    : gcvZERO_X;
            }
        }
        break;

#if !defined(COMMON_LITE)
    case glvFLOAT:
        Matrix->type = glvFLOAT;

        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                glmMATFLOAT(Matrix, x, y) = (x == y)
                    ? 1.0f
                    : 0.0f;
            }
        }
        break;
#endif

    default:
        gcmFATAL("_LoadIdentityMatrix: invalid type %d", Type);
    }

    Matrix->identity = GL_TRUE;

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  glfConvertToVivanteMatrix
**
**      Z'c = (Zc + Wc) / 2
**
**      0 < Z'c <= Wc.
**
**  INPUT:
**
**      Matrix
**          Pointer to the matrix to be converted.
**
**  OUTPUT:
**
**      Result
**          Converted matrix.
*/

void glfConvertToVivanteMatrix(
    glsCONTEXT_PTR Context,
    const glsMATRIX_PTR Matrix,
    glsMATRIX_PTR Result
    )
{
    gcmHEADER();

    *Result = *Matrix;

    if (Context->chipModel >= gcv1000 || Context->chipModel == gcv880)
    {
        gcmFOOTER_NO();
        return;
    }

    Result->identity = GL_FALSE;

    switch (Result->type)
    {
    case glvFIXED:
        {
#if !defined(COMMON_LITE)
            GLfloat      z0, z1, z2, z3;
            gltFRACTYPE  matrix[4 * 4];
            GLint        x, y;

            /* Convert to float matrix first. */
            glfGetFromMatrix(Matrix,
                             matrix,
                             glvFRACTYPEENUM
                             );

            for (y = 0; y < 4; y++)
            {
                for (x = 0; x < 4; x++)
                {
                    glmMATFLOAT(Result, x, y) = matrix[x * 4 + y];
                }
            }

            Result->type = glvFLOAT;

            /* Convert to vivante matrix. */
            z0 = glmMATFLOAT(Result, 0, 2) + glmMATFLOAT(Result, 0, 3);
            z1 = glmMATFLOAT(Result, 1, 2) + glmMATFLOAT(Result, 1, 3);
            z2 = glmMATFLOAT(Result, 2, 2) + glmMATFLOAT(Result, 2, 3);
            z3 = glmMATFLOAT(Result, 3, 2) + glmMATFLOAT(Result, 3, 3);

            glmMATFLOAT(Result, 0, 2) = z0 * 0.5f;
            glmMATFLOAT(Result, 1, 2) = z1 * 0.5f;
            glmMATFLOAT(Result, 2, 2) = z2 * 0.5f;
            glmMATFLOAT(Result, 3, 2) = z3 * 0.5f;
#else
            GLfixed z0 = glmMATFIXED(Result, 0, 2) + glmMATFIXED(Result, 0, 3);
            GLfixed z1 = glmMATFIXED(Result, 1, 2) + glmMATFIXED(Result, 1, 3);
            GLfixed z2 = glmMATFIXED(Result, 2, 2) + glmMATFIXED(Result, 2, 3);
            GLfixed z3 = glmMATFIXED(Result, 3, 2) + glmMATFIXED(Result, 3, 3);

            glmMATFIXED(Result, 0, 2) = gcmXMultiply(z0, gcvHALF_X);
            glmMATFIXED(Result, 1, 2) = gcmXMultiply(z1, gcvHALF_X);
            glmMATFIXED(Result, 2, 2) = gcmXMultiply(z2, gcvHALF_X);
            glmMATFIXED(Result, 3, 2) = gcmXMultiply(z3, gcvHALF_X);
#endif
        }
        break;

#if !defined(COMMON_LITE)
    case glvFLOAT:
        {
            GLfloat z0 = glmMATFLOAT(Result, 0, 2) + glmMATFLOAT(Result, 0, 3);
            GLfloat z1 = glmMATFLOAT(Result, 1, 2) + glmMATFLOAT(Result, 1, 3);
            GLfloat z2 = glmMATFLOAT(Result, 2, 2) + glmMATFLOAT(Result, 2, 3);
            GLfloat z3 = glmMATFLOAT(Result, 3, 2) + glmMATFLOAT(Result, 3, 3);

            glmMATFLOAT(Result, 0, 2) = z0 * 0.5f;
            glmMATFLOAT(Result, 1, 2) = z1 * 0.5f;
            glmMATFLOAT(Result, 2, 2) = z2 * 0.5f;
            glmMATFLOAT(Result, 3, 2) = z3 * 0.5f;
        }
        break;
#endif

    default:
        gcmFATAL("Invalid matrix type: %d", Result->type);
    }

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _MultiplyMatrix4x4
**
**  Multiplies two 4x4 matrices.
**
**  INPUT:
**
**      Matrix1
**          Points to the first matrix.
**
**      Matrix2
**          Points to the second matrix.
**
**  OUTPUT:
**
**      Result
**          Product of multiplication.
*/

static gctBOOL _MultiplyMatrix4x4(
    const glsMATRIX_PTR Matrix1,
    const glsMATRIX_PTR Matrix2,
    glsMATRIX_PTR Result
    )
{
    gcmHEADER_ARG("Matrix1=0x%x Matrix2=0x%x Result=0x%x", Matrix1, Matrix2, Result);

    if (Matrix1->identity)
    {
        *Result = *Matrix2;
    }
    else if (Matrix2->identity)
    {
        *Result = *Matrix1;
    }
    else
    {
        GLint x, y;

        Result->identity = GL_FALSE;

        switch (Matrix1->type)
        {
        case glvFIXED:

            switch (Matrix2->type)
            {
            case glvFIXED:
                for (y = 0; y < 4; y++)
                {
                    for (x = 0; x < 4; x++)
                    {
                        glmMATFIXED(Result, x, y) =
	                        gcmXMultiply(
		                        glmMATFIXED(Matrix1, 0, y),
		                        glmMATFIXED(Matrix2, x, 0))
                          + gcmXMultiply(
		                        glmMATFIXED(Matrix1, 1, y),
		                        glmMATFIXED(Matrix2, x, 1))
                          + gcmXMultiply(
		                        glmMATFIXED(Matrix1, 2, y),
		                        glmMATFIXED(Matrix2, x, 2))
                          +	gcmXMultiply(
		                        glmMATFIXED(Matrix1, 3, y),
		                        glmMATFIXED(Matrix2, x, 3));
                    }
                }

                Result->type = glvFIXED;
                break;

#if !defined(COMMON_LITE)
            case glvFLOAT:
                for (y = 0; y < 4; y++)
                {
                    for (x = 0; x < 4; x++)
                    {
                        glmMATFLOAT(Result, x, y) =
	                        (glmFIXED2FLOAT(glmMATFIXED(Matrix1, 0, y))
	                         * glmMATFLOAT(Matrix2, x, 0))
                          + (glmFIXED2FLOAT(glmMATFIXED(Matrix1, 1, y))
	                         * glmMATFLOAT(Matrix2, x, 1))
                          + (glmFIXED2FLOAT(glmMATFIXED(Matrix1, 2, y))
	                         * glmMATFLOAT(Matrix2, x, 2))
                          +	(glmFIXED2FLOAT(glmMATFIXED(Matrix1, 3, y))
	                         * glmMATFLOAT(Matrix2, x, 3));
                    }
                }

                Result->type = glvFLOAT;
                break;
#endif

            default:
                gcmFATAL(
                    "_MultiplyMatrix4x4: invalid second matrix type %d",
                    Matrix2->type
                    );
                gcmFOOTER_NO();
                return gcvFALSE;
            }

            break;

#if !defined(COMMON_LITE)
        case glvFLOAT:

            switch (Matrix2->type)
            {
            case glvFIXED:
                for (y = 0; y < 4; y++)
                {
                    for (x = 0; x < 4; x++)
                    {
                        glmMATFLOAT(Result, x, y) =
	                        (glmMATFLOAT(Matrix1, 0, y)
	                         * glmFIXED2FLOAT(glmMATFIXED(Matrix2, x, 0)))
                          + (glmMATFLOAT(Matrix1, 1, y)
	                         * glmFIXED2FLOAT(glmMATFIXED(Matrix2, x, 1)))
                          + (glmMATFLOAT(Matrix1, 2, y)
	                         * glmFIXED2FLOAT(glmMATFIXED(Matrix2, x, 2)))
                          +	(glmMATFLOAT(Matrix1, 3, y)
	                         * glmFIXED2FLOAT(glmMATFIXED(Matrix2, x, 3)));
                    }
                }

                Result->type = glvFLOAT;
                break;

            case glvFLOAT:
                for (y = 0; y < 4; y++)
                {
                    for (x = 0; x < 4; x++)
                    {
                        glmMATFLOAT(Result, x, y) =
	                        (glmMATFLOAT(Matrix1, 0, y)
	                         * glmMATFLOAT(Matrix2, x, 0))
                          + (glmMATFLOAT(Matrix1, 1, y)
	                         * glmMATFLOAT(Matrix2, x, 1))
                          + (glmMATFLOAT(Matrix1, 2, y)
	                         * glmMATFLOAT(Matrix2, x, 2))
                          +	(glmMATFLOAT(Matrix1, 3, y)
	                         * glmMATFLOAT(Matrix2, x, 3));
                    }
                }

                Result->type = glvFLOAT;
                break;

            default:
                gcmFATAL(
                    "_MultiplyMatrix4x4: invalid second matrix type %d",
                    Matrix2->type
                    );
                gcmFOOTER_NO();
                return gcvFALSE;
            }

            break;
#endif

        default:
            gcmFATAL(
                "_MultiplyMatrix4x4: invalid first matrix type %d",
                Matrix1->type
                );
            gcmFOOTER_NO();
            return gcvFALSE;
        }
    }
    gcmFOOTER_NO();
    return gcvTRUE;
}


/*******************************************************************************
**
**  _TransposeMatrix
**
**  Transposed matrix is the original matrix flipped along its main diagonal.
**
**  INPUT:
**
**      Matrix
**          Points to the matrix to be transposed.
**
**  OUTPUT:
**
**      Result
**          Transposed matrix.
*/

static void _TransposeMatrix(
    const glsMATRIX_PTR Matrix,
    glsMATRIX_PTR Result
    )
{
    GLint y, x;

    gcmHEADER_ARG("Matrix=0x%x Result=0x%x", Matrix, Result);

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 4; x++)
        {
            glmMAT(Result, x, y) = glmMAT(Matrix, y, x);
        }
    }

    Result->type     = Matrix->type;
    Result->identity = Matrix->identity;

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _InverseMatrix3x3
**
**  Get an inverse 3x3 matrix by Gaussian-Jordan Elimination.
**
**  INPUT:
**
**      Row0, Row1, Row2
**          Pointers to the rows of the augmented matrix.
**
**  OUTPUT:
**
**      Result
**          Result of the inversed matrix.
*/

static GLboolean _InverseFixedMatrix3x3(
    gluMUTABLE Row0[8],
    gluMUTABLE Row1[8],
    gluMUTABLE Row2[8],
    glsMATRIX_PTR Result
    )
{
    GLint i;
    gcmHEADER_ARG("Row0=0x%x Row1=0x%x Row2=0x%x Result=0x%x",
                    Row0, Row1, Row2, Result);

    /***************************************************************************
    ** Transform the first column to (x, 0, 0).
    */

    /* Make sure [0][0] is not a zero. */
    if (glmABS(Row2[0].x) > glmABS(Row1[0].x)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (glmABS(Row1[0].x) > glmABS(Row0[0].x)) glmSWAP(gluMUTABLE_PTR, Row1, Row0);
    if (Row0[0].x == 0)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 1; i < 6; i++)
    {
        if (Row0[i].x != 0)
        {
            if (Row1[0].x != 0)
            {
                Row1[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row0[i].x, Row1[0].x, Row0[0].x
                    );
            }

            if (Row2[0].x != 0)
            {
                Row2[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row0[i].x, Row2[0].x, Row0[0].x
                    );
            }
        }
    }


    /***************************************************************************
    ** Transform the second column to (0, x, 0).
    */

    /* Make sure [1][1] is not a zero. */
    if (glmABS(Row2[1].x) > glmABS(Row1[1].x)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (Row1[1].x == 0)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 2; i < 6; i++)
    {
        if (Row1[i].x != 0)
        {
            if (Row0[1].x != 0)
            {
                Row0[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row1[i].x, Row0[1].x, Row1[1].x
                    );
            }

            if (Row2[1].x != 0)
            {
                Row2[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row1[i].x, Row2[1].x, Row1[1].x
                    );
            }
        }
    }


    /***************************************************************************
    ** Transform the third column to (0, 0, x).
    */

    /* Make sure [2][2] is not a zero. */
    if (Row2[2].x == 0)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 3; i < 6; i++)
    {
        if (Row2[i].x != 0)
        {
            if (Row0[2].x != 0)
            {
                Row0[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row2[i].x, Row0[2].x, Row2[2].x
                    );
            }

            if (Row1[2].x != 0)
            {
                Row1[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row2[i].x, Row1[2].x, Row2[2].x
                    );
            }
        }
    }


    /***************************************************************************
    ** Normalize the result.
    */

    /* Advance to the inverse result. */
    for (i = 0; i < 3; i++)
    {
        glmMATFIXED(Result, i, 0) = glmFIXEDDIVIDE(Row0[i + 3].x, Row0[0].x);
        glmMATFIXED(Result, i, 1) = glmFIXEDDIVIDE(Row1[i + 3].x, Row1[1].x);
        glmMATFIXED(Result, i, 2) = glmFIXEDDIVIDE(Row2[i + 3].x, Row2[2].x);
    }

    gcmFOOTER_ARG("%d", GL_TRUE);
    /* Success. */
    return GL_TRUE;
}

#if !defined(COMMON_LITE)
static GLboolean _InverseFloatMatrix3x3(
    gluMUTABLE Row0[8],
    gluMUTABLE Row1[8],
    gluMUTABLE Row2[8],
    glsMATRIX_PTR Result
    )
{
    GLint i;
    gcmHEADER_ARG("Row0=0x%x Row1=0x%x Row2=0x%x Result=0x%x",
                    Row0, Row1, Row2, Result);

    /***************************************************************************
    ** Transform the first column to (x, 0, 0).
    */

    /* Make sure [0][0] is not a zero. */
    if (glmABS(Row2[0].f) > glmABS(Row1[0].f)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (glmABS(Row1[0].f) > glmABS(Row0[0].f)) glmSWAP(gluMUTABLE_PTR, Row1, Row0);
    if (Row0[0].f == 0.0f)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 1; i < 6; i++)
    {
        if (Row0[i].f != 0.0f)
        {
            if (Row1[0].f != 0.0f)
            {
                Row1[i].f -= Row0[i].f * Row1[0].f / Row0[0].f;
            }

            if (Row2[0].f != 0.0f)
            {
                Row2[i].f -= Row0[i].f * Row2[0].f / Row0[0].f;
            }
        }
    }


    /***************************************************************************
    ** Transform the second column to (0, x, 0).
    */

    /* Make sure [1][1] is not a zero. */
    if (glmABS(Row2[1].f) > glmABS(Row1[1].f)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (Row1[1].f == 0.0f)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 2; i < 6; i++)
    {
        if (Row1[i].f != 0.0f)
        {
            if (Row0[1].f != 0.0f)
            {
                Row0[i].f -= Row1[i].f * Row0[1].f / Row1[1].f;
            }

            if (Row2[1].f != 0.0f)
            {
                Row2[i].f -= Row1[i].f * Row2[1].f / Row1[1].f;
            }
        }
    }


    /***************************************************************************
    ** Transform the third column to (0, 0, x).
    */

    /* Make sure [2][2] is not a zero. */
    if (Row2[2].f == 0.0f)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 3; i < 6; i++)
    {
        if (Row2[i].f != 0.0f)
        {
            if (Row0[2].f != 0.0f)
            {
                Row0[i].f -= Row2[i].f * Row0[2].f / Row2[2].f;
            }

            if (Row1[2].f != 0.0f)
            {
                Row1[i].f -= Row2[i].f * Row1[2].f / Row2[2].f;
            }
        }
    }


    /***************************************************************************
    ** Normalize the result.
    */

    for (i = 0; i < 3; i++)
    {
        glmMATFLOAT(Result, i, 0) = Row0[i + 3].f / Row0[0].f;
        glmMATFLOAT(Result, i, 1) = Row1[i + 3].f / Row1[1].f;
        glmMATFLOAT(Result, i, 2) = Row2[i + 3].f / Row2[2].f;
    }

    gcmFOOTER_ARG("%d", GL_TRUE);
    /* Success. */
    return GL_TRUE;
}
#endif


/*******************************************************************************
**
**  _InverseMatrix3x3
**
**  Get an inversed 3x3 matrix using Gaussian-Jordan Elimination.
**
**  INPUT:
**
**      Matrix
**          Points to the matrix to be inversed.
**
**  OUTPUT:
**
**      Result
**          Result of the inversed matrix.
*/

static GLboolean _InverseMatrix3x3(
    const glsMATRIX_PTR Matrix,
    glsMATRIX_PTR Result
    )
{
    GLboolean result;
    gcmHEADER_ARG("Matrix=0x%x Result=0x%x", Matrix, Result);

    if (Matrix->identity)
    {
        *Result = *Matrix;
        result = GL_TRUE;
    }
    else
    {
        gluMUTABLE row0[6];
        gluMUTABLE row1[6];
        gluMUTABLE row2[6];

        GLint i;

        /* Initialize the augmented matrix. */
        for (i = 0; i < 3; i++)
        {
            row0[i] = glmMAT(Matrix, i, 0);
            row1[i] = glmMAT(Matrix, i, 1);
            row2[i] = glmMAT(Matrix, i, 2);

            row0[i + 3].x = 0;
            row1[i + 3].x = 0;
            row2[i + 3].x = 0;
        }

        switch (Matrix->type)
        {
        case glvFIXED:
            row0[3].x = gcvONE_X;
            row1[4].x = gcvONE_X;
            row2[5].x = gcvONE_X;

            result = _InverseFixedMatrix3x3(row0, row1, row2, Result);
            break;

#if !defined(COMMON_LITE)
        case glvFLOAT:
            row0[3].f = 1.0f;
            row1[4].f = 1.0f;
            row2[5].f = 1.0f;

            result = _InverseFloatMatrix3x3(row0, row1, row2, Result);
            break;
#endif

        default:
            gcmFATAL("_InverseMatrix3x3: invalid type %d", Matrix->type);
            result = GL_FALSE;
        }

        if (result)
        {
            Result->type = Matrix->type;
            Result->identity = GL_FALSE;

            for (i = 0; i < 4; i++)
            {
                glmMAT(Result, i, 3).i = 0;
                glmMAT(Result, 3, i).i = 0;
            }
        }
    }

    gcmFOOTER_ARG("%d", result);
    return result;
}


/*******************************************************************************
**
**  _InverseMatrix4x4
**
**  Get an inverse 4x4 matrix by Gaussian-Jordan Elimination.
**
**  INPUT:
**
**      Row0, Row1, Row2, Row4
**          Pointers to the rows of the augmented matrix.
**
**  OUTPUT:
**
**      Result
**          Result of the inversed matrix.
*/

static GLboolean _InverseFixedMatrix4x4(
    gluMUTABLE Row0[8],
    gluMUTABLE Row1[8],
    gluMUTABLE Row2[8],
    gluMUTABLE Row3[8],
    glsMATRIX_PTR Result
    )
{
    GLint i;
    gcmHEADER_ARG("Row0=0x%x Row1=0x%x Row2=0x%x Row3=0x%x Result=0x%x",
                    Row0, Row1, Row2, Row3, Result);

    /***************************************************************************
    ** Transform the first column to (x, 0, 0, 0).
    */

    /* Make sure [0][0] is not a zero. */
    if (glmABS(Row3[0].x) > glmABS(Row2[0].x)) glmSWAP(gluMUTABLE_PTR, Row3, Row2);
    if (glmABS(Row2[0].x) > glmABS(Row1[0].x)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (glmABS(Row1[0].x) > glmABS(Row0[0].x)) glmSWAP(gluMUTABLE_PTR, Row1, Row0);
    if (Row0[0].x == 0)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 1; i < 8; i++)
    {
        if (Row0[i].x != 0)
        {
            if (Row1[0].x != 0)
            {
                Row1[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row0[i].x, Row1[0].x, Row0[0].x
                    );
            }

            if (Row2[0].x != 0)
            {
                Row2[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row0[i].x, Row2[0].x, Row0[0].x
                    );
            }

            if (Row3[0].x != 0)
            {
                Row3[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row0[i].x, Row3[0].x, Row0[0].x
                    );
            }
        }
    }


    /***************************************************************************
    ** Transform the second column to (0, x, 0, 0).
    */

    /* Make sure [1][1] is not a zero. */
    if (glmABS(Row3[1].x) > glmABS(Row2[1].x)) glmSWAP(gluMUTABLE_PTR, Row3, Row2);
    if (glmABS(Row2[1].x) > glmABS(Row1[1].x)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (Row1[1].x == 0)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 2; i < 8; i++)
    {
        if (Row1[i].x != 0)
        {
            if (Row0[1].x != 0)
            {
                Row0[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row1[i].x, Row0[1].x, Row1[1].x
                    );
            }

            if (Row2[1].x != 0)
            {
                Row2[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row1[i].x, Row2[1].x, Row1[1].x
                    );
            }

            if (Row3[1].x != 0)
            {
                Row3[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row1[i].x, Row3[1].x, Row1[1].x
                    );
            }
        }
    }


    /***************************************************************************
    ** Transform the third column to (0, 0, x, 0).
    */

    /* Make sure [2][2] is not a zero. */
    if (glmABS(Row3[2].x) > glmABS(Row2[2].x)) glmSWAP(gluMUTABLE_PTR, Row3, Row2);
    if (Row2[2].x == 0)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 3; i < 8; i++)
    {
        if (Row2[i].x != 0)
        {
            if (Row0[2].x != 0)
            {
                Row0[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row2[i].x, Row0[2].x, Row2[2].x
                    );
            }

            if (Row1[2].x != 0)
            {
                Row1[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row2[i].x, Row1[2].x, Row2[2].x
                    );
            }

            if (Row3[2].x != 0)
            {
                Row3[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row2[i].x, Row3[2].x, Row2[2].x
                    );
            }
        }
    }


    /***************************************************************************
    ** Transform the fourth column to (0, 0, 0, x).
    */

    if (Row3[3].x == 0)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 4; i < 8; i++)
    {
        if (Row3[i].x != 0)
        {
            if (Row0[3].x != 0)
            {
                Row0[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row3[i].x, Row0[3].x, Row3[3].x
                    );
            }

            if (Row1[3].x != 0)
            {
                Row1[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row3[i].x, Row1[3].x, Row3[3].x
                    );
            }

            if (Row2[3].x != 0)
            {
                Row2[i].x -= glmFIXEDMULTIPLYDIVIDE(
                    Row3[i].x, Row2[3].x, Row3[3].x
                    );
            }
        }
    }


    /***************************************************************************
    ** Normalize the result.
    */

    /* Advance to the inverse result. */
    for (i = 0; i < 4; i++)
    {
        glmMATFIXED(Result, i, 0) = glmFIXEDDIVIDE(Row0[i + 4].x, Row0[0].x);
        glmMATFIXED(Result, i, 1) = glmFIXEDDIVIDE(Row1[i + 4].x, Row1[1].x);
        glmMATFIXED(Result, i, 2) = glmFIXEDDIVIDE(Row2[i + 4].x, Row2[2].x);
        glmMATFIXED(Result, i, 3) = glmFIXEDDIVIDE(Row3[i + 4].x, Row3[3].x);
    }

    gcmFOOTER_ARG("%d", GL_TRUE);
    /* Success. */
    return GL_TRUE;
}

#if !defined(COMMON_LITE)
static GLboolean _InverseFloatMatrix4x4(
    gluMUTABLE Row0[8],
    gluMUTABLE Row1[8],
    gluMUTABLE Row2[8],
    gluMUTABLE Row3[8],
    glsMATRIX_PTR Result
    )
{
    GLint i;
    gcmHEADER_ARG("Row0=0x%x Row1=0x%x Row2=0x%x Row3=0x%x Result=0x%x",
                    Row0, Row1, Row2, Row3, Result);

    /***************************************************************************
    ** Transform the first column to (x, 0, 0, 0).
    */

    /* Make sure [0][0] is not a zero. */
    if (glmABS(Row3[0].f) > glmABS(Row2[0].f)) glmSWAP(gluMUTABLE_PTR, Row3, Row2);
    if (glmABS(Row2[0].f) > glmABS(Row1[0].f)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (glmABS(Row1[0].f) > glmABS(Row0[0].f)) glmSWAP(gluMUTABLE_PTR, Row1, Row0);
    if (Row0[0].f == 0.0f)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 1; i < 8; i++)
    {
        if (Row0[i].f != 0.0f)
        {
            if (Row1[0].f != 0.0f)
            {
                Row1[i].f -= Row0[i].f * Row1[0].f / Row0[0].f;
            }

            if (Row2[0].f != 0.0f)
            {
                Row2[i].f -= Row0[i].f * Row2[0].f / Row0[0].f;
            }

            if (Row3[0].f != 0.0f)
            {
                Row3[i].f -= Row0[i].f * Row3[0].f / Row0[0].f;
            }
        }
    }


    /***************************************************************************
    ** Transform the second column to (0, x, 0, 0).
    */

    /* Make sure [1][1] is not a zero. */
    if (glmABS(Row3[1].f) > glmABS(Row2[1].f)) glmSWAP(gluMUTABLE_PTR, Row3, Row2);
    if (glmABS(Row2[1].f) > glmABS(Row1[1].f)) glmSWAP(gluMUTABLE_PTR, Row2, Row1);
    if (Row1[1].f == 0.0f)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 2; i < 8; i++)
    {
        if (Row1[i].f != 0.0f)
        {
            if (Row0[1].f != 0.0f)
            {
                Row0[i].f -= Row1[i].f * Row0[1].f / Row1[1].f;
            }

            if (Row2[1].f != 0.0f)
            {
                Row2[i].f -= Row1[i].f * Row2[1].f / Row1[1].f;
            }

            if (Row3[1].f != 0.0f)
            {
                Row3[i].f -= Row1[i].f * Row3[1].f / Row1[1].f;
            }
        }
    }


    /***************************************************************************
    ** Transform the third column to (0, 0, x, 0).
    */

    /* Make sure [2][2] is not a zero. */
    if (glmABS(Row3[2].f) > glmABS(Row2[2].f)) glmSWAP(gluMUTABLE_PTR, Row3, Row2);
    if (Row2[2].f == 0.0f)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 3; i < 8; i++)
    {
        if (Row2[i].f != 0.0f)
        {
            if (Row0[2].f != 0.0f)
            {
                Row0[i].f -= Row2[i].f * Row0[2].f / Row2[2].f;
            }

            if (Row1[2].f != 0.0f)
            {
                Row1[i].f -= Row2[i].f * Row1[2].f / Row2[2].f;
            }

            if (Row3[2].f != 0.0f)
            {
                Row3[i].f -= Row2[i].f * Row3[2].f / Row2[2].f;
            }
        }
    }


    /***************************************************************************
    ** Transform the fourth column to (0, 0, 0, x).
    */

    if (Row3[3].f == 0.0f)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    for (i = 4; i < 8; i++)
    {
        if (Row3[i].f != 0.0f)
        {
            if (Row0[3].f != 0.0f)
            {
                Row0[i].f -= Row3[i].f * Row0[3].f / Row3[3].f;
            }

            if (Row1[3].f != 0.0f)
            {
                Row1[i].f -= Row3[i].f * Row1[3].f / Row3[3].f;
            }

            if (Row2[3].f != 0.0f)
            {
                Row2[i].f -= Row3[i].f * Row2[3].f / Row3[3].f;
            }
        }
    }


    /***************************************************************************
    ** Normalize the result.
    */

    for (i = 0; i < 4; i++)
    {
        glmMATFLOAT(Result, i, 0) = Row0[i + 4].f / Row0[0].f;
        glmMATFLOAT(Result, i, 1) = Row1[i + 4].f / Row1[1].f;
        glmMATFLOAT(Result, i, 2) = Row2[i + 4].f / Row2[2].f;
        glmMATFLOAT(Result, i, 3) = Row3[i + 4].f / Row3[3].f;
    }

    gcmFOOTER_ARG("%d", GL_TRUE);
    /* Success. */
    return GL_TRUE;
}
#endif


/*******************************************************************************
**
**  _InverseMatrix4x4
**
**  Get an inversed 4x4 matrix using Gaussian-Jordan Elimination.
**
**  INPUT:
**
**      Matrix
**          Points to the matrix to be inversed.
**
**  OUTPUT:
**
**      Result
**          Result of the inversed matrix.
*/

static GLboolean _InverseMatrix4x4(
    const glsMATRIX_PTR Matrix,
    glsMATRIX_PTR Result
    )
{
    GLboolean result;

    gluMUTABLE row0[8];
    gluMUTABLE row1[8];
    gluMUTABLE row2[8];
    gluMUTABLE row3[8];

    GLint i;

    gcmHEADER_ARG("Matrix=0x%x Result=0x%x", Matrix, Result);

    /* Initialize the augmented matrix. */
    for (i = 0; i < 4; i++)
    {
        row0[i] = glmMAT(Matrix, i, 0);
        row1[i] = glmMAT(Matrix, i, 1);
        row2[i] = glmMAT(Matrix, i, 2);
        row3[i] = glmMAT(Matrix, i, 3);

        row0[i + 4].x = 0;
        row1[i + 4].x = 0;
        row2[i + 4].x = 0;
        row3[i + 4].x = 0;
    }

    switch (Matrix->type)
    {
    case glvFIXED:
        row0[4].x = gcvONE_X;
        row1[5].x = gcvONE_X;
        row2[6].x = gcvONE_X;
        row3[7].x = gcvONE_X;

        result = _InverseFixedMatrix4x4(row0, row1, row2, row3, Result);
        break;

#if !defined(COMMON_LITE)
    case glvFLOAT:
        row0[4].f = 1.0f;
        row1[5].f = 1.0f;
        row2[6].f = 1.0f;
        row3[7].f = 1.0f;

        result = _InverseFloatMatrix4x4(row0, row1, row2, row3, Result);
        break;
#endif

    default:
        gcmFATAL("_InverseMatrix4x4: invalid type %d", Matrix->type);
        result = GL_FALSE;
    }

    Result->type = Matrix->type;
    Result->identity = GL_FALSE;

    gcmFOOTER_ARG("%d", result);
    return result;
}


/*******************************************************************************
**
**  _LoadMatrix
**
**  Loads matrix from an array of raw values:
**
**      m[0]        m[4]        m[8]        m[12]
**      m[1]        m[5]        m[9]        m[13]
**      m[2]        m[6]        m[10]       m[14]
**      m[3]        m[7]        m[11]       m[15]
**
**  INPUT:
**
**      Matrix
**          Pointer to the matrix to be reset.
**
**      Type
**          Type of the matrix to be produced.
**
**      Values
**          Points to the array of values.
**
**  OUTPUT:
**
**      Matrix
**          Identity matrix.
*/

static gctBOOL _LoadMatrix(
    glsMATRIX_PTR Matrix,
    gleTYPE Type,
    const GLvoid* Values
    )
{
    GLint x, y, i;
    gctBOOL result;

    gcmHEADER_ARG("Matrix=0x%x Type=0x%04x Values=0x%x", Matrix, Type, Values);

    switch (Type)
    {
    case glvFIXED:
        Matrix->type = glvFIXED;

        for (i = 0, x = 0; x < 4; x++)
        {
            for (y = 0; y < 4; y++, i++)
            {
                glmMATFIXED(Matrix, x, y) = ((GLfixed *) Values) [i];
            }
        }
        break;

#if !defined(COMMON_LITE)
    case glvFLOAT:
        Matrix->type = glvFLOAT;

        gcmVERIFY_OK(gcoOS_MemCopy(&Matrix->value, Values, 4 * 4 * gcmSIZEOF(GLfloat)));
        break;
#endif

    default:
        gcmFATAL("_LoadMatrix: invalid type %d", Type);
        gcmFOOTER_NO();
        return gcvFALSE;
    }

    /* Update matrix flags. */
    result = _UpdateMatrixFlags(Matrix);
    gcmFOOTER_NO();
    return result;
}


/******************************************************************************\
******************** Model View matrix modification handlers *******************
\******************************************************************************/

/* Current model view matrix has changed, update pointers. */
static void _ModelViewMatrixCurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->modelViewMatrix
        = Context->matrixStackArray[glvMODEL_VIEW_MATRIX].topMatrix;

    /* Set uModelView, uModeViewInversed uModelViewProjection dirty. */
    Context->vsUniformDirty.uModelViewDirty                     = gcvTRUE;
    Context->vsUniformDirty.uModelViewInverse3x3TransposedDirty = gcvTRUE;
    Context->vsUniformDirty.uModeViewProjectionDirty            = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current model view matrix has changed, invalidate dependets. */
static void _ModelViewMatrixDataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    Context->hashKey.hashModelViewIdentity =
        Context->modelViewMatrix->identity;

    Context->modelViewInverse3x3TransposedMatrix.recompute = GL_TRUE;
    Context->modelViewInverse3x3TransposedMatrix.dirty     = GL_TRUE;
    Context->modelViewInverse4x4TransposedMatrix.recompute = GL_TRUE;
    Context->modelViewProjectionMatrix.recompute = GL_TRUE;
    Context->modelViewProjectionMatrix.dirty     = GL_TRUE;

    /* Set uModelView, uModeViewInversed uModelViewProjection dirty. */
    Context->vsUniformDirty.uModelViewDirty                     = gcvTRUE;
    Context->vsUniformDirty.uModelViewInverse3x3TransposedDirty = gcvTRUE;
    Context->vsUniformDirty.uModeViewProjectionDirty            = gcvTRUE;

    gcmFOOTER_NO();
}


/******************************************************************************\
******************** Projection matrix modification handlers *******************
\******************************************************************************/

/* Current projection matrix has changed, update pointers. */
static void _ProjectionMatrixCurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->projectionMatrix
        = Context->matrixStackArray[glvPROJECTION_MATRIX].topMatrix;

    /* Set uProjection and uModelViewProjection dirty. */
    Context->vsUniformDirty.uProjectionDirty         = gcvTRUE;
    Context->vsUniformDirty.uModeViewProjectionDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current projection matrix has changed, invalidate dependets. */
static void _ProjectionMatrixDataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->hashKey.hashProjectionIdentity =
        Context->projectionMatrix->identity;

    Context->convertedProjectionMatrix.recompute = GL_TRUE;
    Context->modelViewProjectionMatrix.recompute = GL_TRUE;
    Context->modelViewProjectionMatrix.dirty     = GL_TRUE;

    /* Set uProjection and uModelViewProjection dirty. */
    Context->vsUniformDirty.uProjectionDirty         = gcvTRUE;
    Context->vsUniformDirty.uModeViewProjectionDirty = gcvTRUE;

    gcmFOOTER_NO();
}


/******************************************************************************\
********************* Palette matrix modification handlers *********************
\******************************************************************************/

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix0CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix0DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[0].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix1CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix1DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[1].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix2CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix2DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[2].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix3CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix3DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[3].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix4CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix4DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[4].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;
    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix5CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix5DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[5].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix6CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix6DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[6].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix7CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix7DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[7].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix8CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix8DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[8].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

#if glvMAX_PALETTE_MATRICES > 9
/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix9CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix9DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[9].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;
    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix10CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix10DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[10].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix11CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix11DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[11].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix12CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix12DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[12].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix13CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix13DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[13].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix14CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix14DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[14].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix15CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix15DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[15].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix16CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix16DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[16].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix17CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix17DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[17].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix18CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix18DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[18].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix19CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix19DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[19].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix20CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix20DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[20].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix21CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix21DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[21].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix22CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix22DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[22].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix23CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix23DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[23].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix24CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix24DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[24].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix25CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix25DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[25].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix26CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix26DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[26].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix27CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix27DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[27].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix28CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix28DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[28].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix29CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix29DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[29].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix30CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix30DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[30].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current palette matrix has changed, update pointers. */
static void _PaletteMatrix31CurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current palette matrix has changed, invalidate dependets. */
static void _PaletteMatrix31DataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->paletteMatrixInverse3x3[31].recompute = GL_TRUE;
    Context->paletteMatrixInverse3x3Recompute = GL_TRUE;

    /* Set uMatrixPalette dirty. */
    Context->vsUniformDirty.uMatrixPaletteDirty        = gcvTRUE;
    Context->vsUniformDirty.uMatrixPaletteInverseDirty = gcvTRUE;

    gcmFOOTER_NO();
}
#endif

/******************************************************************************\
********************* Texture matrix modification handlers *********************
\******************************************************************************/

/* Current texture 0 matrix has changed, update pointers. */
static void _Texture0MatrixCurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->texture.activeSamplerIndex == 0)
    {
        Context->textureMatrix
            = Context->matrixStackArray[glvTEXTURE_MATRIX_0].topMatrix;
    }

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current texture 0 matrix has changed, invalidate dependets. */
static void _Texture0MatrixDataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    /* Update the hash key. */
    glmSETHASH_1BIT(hashTextureIdentity, Context->textureMatrix->identity, 0);

    /* Invalidate current texture coordinate. */
    Context->texture.sampler[0].recomputeCoord = GL_TRUE;
    Context->texture.matrixDirty = GL_TRUE;

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current texture 1 matrix has changed, update pointers. */
static void _Texture1MatrixCurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->texture.activeSamplerIndex == 1)
    {
        Context->textureMatrix
            = Context->matrixStackArray[glvTEXTURE_MATRIX_1].topMatrix;
    }

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current texture 1 matrix has changed, invalidate dependets. */
static void _Texture1MatrixDataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    /* Update the hash key. */
    glmSETHASH_1BIT(hashTextureIdentity, Context->textureMatrix->identity, 1);

    /* Invalidate current texture coordinate. */
    Context->texture.sampler[1].recomputeCoord = GL_TRUE;
    Context->texture.matrixDirty = GL_TRUE;

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current texture 2 matrix has changed, update pointers. */
static void _Texture2MatrixCurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->texture.activeSamplerIndex == 2)
    {
        Context->textureMatrix
            = Context->matrixStackArray[glvTEXTURE_MATRIX_2].topMatrix;
    }

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current texture 2 matrix has changed, invalidate dependets. */
static void _Texture2MatrixDataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    /* Update the hash key. */
    glmSETHASH_1BIT(hashTextureIdentity, Context->textureMatrix->identity, 2);

    /* Invalidate current texture coordinate. */
    Context->texture.sampler[2].recomputeCoord = GL_TRUE;
    Context->texture.matrixDirty = GL_TRUE;

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Current texture 3 matrix has changed, update pointers. */
static void _Texture3MatrixCurrentChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->texture.activeSamplerIndex == 3)
    {
        Context->textureMatrix
            = Context->matrixStackArray[glvTEXTURE_MATRIX_3].topMatrix;
    }

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}

/* Value of the current texture 3 matrix has changed, invalidate dependets. */
static void _Texture3MatrixDataChanged(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    /* Update the hash key. */
    glmSETHASH_1BIT(hashTextureIdentity, Context->textureMatrix->identity, 3);

    /* Invalidate current texture coordinate. */
    Context->texture.sampler[3].recomputeCoord = GL_TRUE;
    Context->texture.matrixDirty = GL_TRUE;

    /* Set uTexMatrix dirty. */
    Context->vsUniformDirty.uTexMatrixDirty = gcvTRUE;

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _InitializeMatrixStack
**
**  Allocate matrix staack and initialize all matrices to identity.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      MatrixStack
**          Pointer to the matrix stack container.
**
**      Count
**          Number of matrixes in the stack.
**
**      NotifyCallback
**          Function to call when there is a change on the stack.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _InitializeMatrixStack(
    glsCONTEXT_PTR Context,
    glsMATRIXSTACK_PTR MatrixStack,
    GLuint Count,
    glfMATRIXCHANGEEVENT CurrentChanged,
    glfMATRIXCHANGEEVENT DataChanged
    )
{
    GLuint i;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x MatrixStack=0x%x Count=%u CurrentChanged=0x%x DataChanged=0x%x",
                    Context, MatrixStack, Count, CurrentChanged, DataChanged);

    do
    {
        GLuint stackSize = sizeof(glsMATRIX) * Count;
        gctPOINTER pointer = gcvNULL;

        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            stackSize,
            &pointer
            ));

        MatrixStack->stack = pointer;

        gcoOS_ZeroMemory(MatrixStack->stack, stackSize);

        MatrixStack->count = Count;
        MatrixStack->index = 0;
        MatrixStack->topMatrix = MatrixStack->stack;
        MatrixStack->currChanged = CurrentChanged;
        MatrixStack->dataChanged = DataChanged;

        for (i = 0; i < MatrixStack->count; i++)
        {
            glsMATRIX_PTR matrix = &MatrixStack->stack[i];
            _LoadIdentityMatrix(matrix, glvFRACTYPEENUM);
        }

        (*MatrixStack->currChanged) (Context);
        (*MatrixStack->dataChanged) (Context);
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  _FreeMatrixStack
**
**  Free allocated matrix stack.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      MatrixStack
**          Pointer to the matrix stack container.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _FreeMatrixStack(
    glsCONTEXT_PTR Context,
    glsMATRIXSTACK_PTR MatrixStack
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x MatrixStack=0x%x", Context, MatrixStack);

    if (MatrixStack->stack != gcvNULL)
    {
        status = gcmOS_SAFE_FREE(gcvNULL, MatrixStack->stack);
    }
    else
    {
        status = gcvSTATUS_OK;
    }

    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfInitializeMatrixStack
**
**  Initialize matrix stacks.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfInitializeMatrixStack(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        struct _glsSTACKINFO
        {
            gctINT matrixCount;
            glfMATRIXCHANGEEVENT currChanged;
            glfMATRIXCHANGEEVENT dataChanged;
        };

        static struct _glsSTACKINFO stackInfo[glvSTACKCOUNT] =
        {
            /* Model View matrix stack. */
            {
                glvMAX_STACK_NUM_MODELVIEW,
                    _ModelViewMatrixCurrentChanged,
                    _ModelViewMatrixDataChanged
            },

            /* Projection matrix stack. */
            {
                glvMAX_STACK_NUM_PROJECTION,
                    _ProjectionMatrixCurrentChanged,
                    _ProjectionMatrixDataChanged
            },

            /* Palette matrix stack. */
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix0CurrentChanged,
                    _PaletteMatrix0DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix1CurrentChanged,
                    _PaletteMatrix1DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix2CurrentChanged,
                    _PaletteMatrix2DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix3CurrentChanged,
                    _PaletteMatrix3DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix4CurrentChanged,
                    _PaletteMatrix4DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix5CurrentChanged,
                    _PaletteMatrix5DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix6CurrentChanged,
                    _PaletteMatrix6DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix7CurrentChanged,
                    _PaletteMatrix7DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix8CurrentChanged,
                    _PaletteMatrix8DataChanged
            },
#if glvMAX_PALETTE_MATRICES > 9
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix9CurrentChanged,
                    _PaletteMatrix9DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix10CurrentChanged,
                    _PaletteMatrix10DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix11CurrentChanged,
                    _PaletteMatrix11DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix12CurrentChanged,
                    _PaletteMatrix12DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix13CurrentChanged,
                    _PaletteMatrix13DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix14CurrentChanged,
                    _PaletteMatrix14DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix15CurrentChanged,
                    _PaletteMatrix15DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix16CurrentChanged,
                    _PaletteMatrix16DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix17CurrentChanged,
                    _PaletteMatrix17DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix18CurrentChanged,
                    _PaletteMatrix18DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix19CurrentChanged,
                    _PaletteMatrix19DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix20CurrentChanged,
                    _PaletteMatrix20DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix21CurrentChanged,
                    _PaletteMatrix21DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix22CurrentChanged,
                    _PaletteMatrix22DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix23CurrentChanged,
                    _PaletteMatrix23DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix24CurrentChanged,
                    _PaletteMatrix24DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix25CurrentChanged,
                    _PaletteMatrix25DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix26CurrentChanged,
                    _PaletteMatrix26DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix27CurrentChanged,
                    _PaletteMatrix27DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix28CurrentChanged,
                    _PaletteMatrix28DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix29CurrentChanged,
                    _PaletteMatrix29DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix30CurrentChanged,
                    _PaletteMatrix30DataChanged
            },
            {
                glvMAX_STACK_NUM_PALETTE,
                    _PaletteMatrix31CurrentChanged,
                    _PaletteMatrix31DataChanged
            },
#endif
            /* Texture matrix stack. */
            {
                glvMAX_STACK_NUM_TEXTURES,
                    _Texture0MatrixCurrentChanged,
                    _Texture0MatrixDataChanged
            },
            {
                glvMAX_STACK_NUM_TEXTURES,
                    _Texture1MatrixCurrentChanged,
                    _Texture1MatrixDataChanged
            },
            {
                glvMAX_STACK_NUM_TEXTURES,
                    _Texture2MatrixCurrentChanged,
                    _Texture2MatrixDataChanged
            },
            {
                glvMAX_STACK_NUM_TEXTURES,
                    _Texture3MatrixCurrentChanged,
                    _Texture3MatrixDataChanged
            },
        };

        gctINT i;

        for (i = 0; i < glvSTACKCOUNT; i++)
        {
            gcmERR_BREAK(_InitializeMatrixStack(
                Context,
                &Context->matrixStackArray[i],
                stackInfo[i].matrixCount,
                stackInfo[i].currChanged,
                stackInfo[i].dataChanged
                ));
        }

        if (gcmIS_ERROR(status))
        {
            break;
        }

        Context->modelViewProjectionMatrix.dirty = GL_TRUE;

        status = glmTRANSLATEGLRESULT(glfSetMatrixMode(
            Context,
            GL_MODELVIEW
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfFreeMatrixStack
**
**  Free allocated matrix stack.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      MatrixStack
**          Pointer to the matrix stack container.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfFreeMatrixStack(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    do
    {
        gctINT i;

        for (i = 0; i < glvSTACKCOUNT; i++)
        {
            gceSTATUS last = _FreeMatrixStack(
                Context,
                &Context->matrixStackArray[i]
                );

            if (gcmIS_ERROR(last))
            {
                status = last;
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfSetMatrixMode
**
**  Set current matrix mode.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      MatrixMode
**          Matrix mode to be set.
**
**  OUTPUT:
**
**      Nothing.
**
*/

GLenum glfSetMatrixMode(
    glsCONTEXT_PTR Context,
    GLenum MatrixMode
    )
{
    GLenum result = GL_NO_ERROR;

    gcmHEADER();

    switch (MatrixMode)
    {
    case GL_MODELVIEW:
        Context->matrixMode = glvMODEL_VIEW_MATRIX;
        break;

    case GL_PROJECTION:
        Context->matrixMode = glvPROJECTION_MATRIX;
        break;

    case GL_TEXTURE:
        Context->matrixMode
            = (gleMATRIXMODE) (glvTEXTURE_MATRIX_0
            + Context->texture.activeSamplerIndex);
        break;

    case GL_MATRIX_PALETTE_OES:
        Context->matrixMode
            = (gleMATRIXMODE) (glvPALETTE_MATRIX_0
            + Context->currentPalette);
        break;

    default:
        result = GL_INVALID_ENUM;
    }

    if (result == GL_NO_ERROR)
    {
        Context->currentStack = &Context->matrixStackArray[Context->matrixMode];
        Context->currentMatrix = Context->currentStack->topMatrix;
    }

    gcmFOOTER_ARG("result=0x%04x", result);
    return result;
}


/*******************************************************************************
**
**  glfGetModelViewInverse3x3TransposedMatrix
**
**  Get the current model view inverse transposed 3x3 matrix.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Inversed Model View matrix.
*/

glsMATRIX_PTR glfGetModelViewInverse3x3TransposedMatrix(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

    if (Context->modelViewInverse3x3TransposedMatrix.recompute
    &&  (Context->modelViewMatrix != gcvNULL)
    )
    {
        glsMATRIX modelViewInverse3x3Matrix;

        if (Context->modelViewMatrix->identity ||
            !_InverseMatrix3x3(
                Context->modelViewMatrix,
                &modelViewInverse3x3Matrix))
        {
            _LoadIdentityMatrix(
                &Context->modelViewInverse3x3TransposedMatrix.matrix,
                Context->modelViewMatrix->type
                );
        }
        else
        {
            _TransposeMatrix(
                &modelViewInverse3x3Matrix,
                &Context->modelViewInverse3x3TransposedMatrix.matrix
                );
        }

        Context->hashKey.hashModelViewInverse3x3TransIdentity =
            Context->modelViewInverse3x3TransposedMatrix.matrix.identity;

        Context->modelViewInverse3x3TransposedMatrix.recompute = GL_FALSE;
    }

    gcmFOOTER_ARG("result=0x%x", &Context->modelViewInverse3x3TransposedMatrix.matrix);
    return &Context->modelViewInverse3x3TransposedMatrix.matrix;
}


/*******************************************************************************
**
**  glfGetModelViewInverse4x4TransposedMatrix
**
**  Get the current model view inverse transposed matrix.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Model View Projection matrix.
*/

glsMATRIX_PTR glfGetModelViewInverse4x4TransposedMatrix(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->modelViewInverse4x4TransposedMatrix.recompute)
    {
        glsMATRIX modelViewInverseMatrix;

        if (Context->modelViewMatrix->identity ||
            !_InverseMatrix4x4(
                Context->modelViewMatrix,
                &modelViewInverseMatrix))
        {
            _LoadIdentityMatrix(
                &Context->modelViewInverse4x4TransposedMatrix.matrix,
                Context->modelViewMatrix->type
                );
        }
        else
        {
            _TransposeMatrix(
                &modelViewInverseMatrix,
                &Context->modelViewInverse4x4TransposedMatrix.matrix
                );
        }

        Context->modelViewInverse4x4TransposedMatrix.recompute = GL_FALSE;
    }

    gcmFOOTER_ARG("result=0x%x", &Context->modelViewInverse4x4TransposedMatrix.matrix);
    return &Context->modelViewInverse4x4TransposedMatrix.matrix;
}


/*******************************************************************************
**
**  glfGetMatrixPaletteInverse
**
**  Get the array of current inversed 3x3 palette matrices.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Array of inversed 3x3 palette matrices.
*/

glsDEPENDENTMATRIX_PTR glfGetMatrixPaletteInverse(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->paletteMatrixInverse3x3Recompute)
    {
        gctUINT i;
        glsMATRIX_PTR paletteMatrix;

        for (i = 0; i < glvMAX_PALETTE_MATRICES; i += 1)
        {
            if (!Context->paletteMatrixInverse3x3[i].recompute)
            {
                continue;
            }

            /* Get the current palette matrix. */
            paletteMatrix
                = Context->matrixStackArray[glvPALETTE_MATRIX_0 + i].topMatrix;

            if (paletteMatrix->identity ||
                !_InverseMatrix3x3(
                    paletteMatrix,
                    &Context->paletteMatrixInverse3x3[i].matrix))
            {
                _LoadIdentityMatrix(
                    &Context->paletteMatrixInverse3x3[i].matrix,
                    paletteMatrix->type
                    );
            }

            Context->paletteMatrixInverse3x3[i].recompute = GL_FALSE;
        }

        Context->paletteMatrixInverse3x3Recompute = GL_FALSE;
    }

    gcmFOOTER_ARG("result=0x%x", Context->paletteMatrixInverse3x3);
    return Context->paletteMatrixInverse3x3;
}


/*******************************************************************************
**
**  glfGetProjectionMatrix
**
**  Get the current converted projection matrix.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**          Projection matrix.
*/

glsMATRIX_PTR glfGetConvertedProjectionMatrix(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->convertedProjectionMatrix.recompute)
    {
        glfConvertToVivanteMatrix(
            Context,
            Context->projectionMatrix,
            &Context->convertedProjectionMatrix.matrix
            );

        Context->convertedProjectionMatrix.recompute = GL_FALSE;
    }

    gcmFOOTER_ARG("result=0x%x", &Context->convertedProjectionMatrix.matrix);
    return &Context->convertedProjectionMatrix.matrix;
}


/*******************************************************************************
**
**  glfGetModelViewProjectionMatrix
**
**  Get the current model view projection matrix.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Model View Projection matrix.
*/

glsMATRIX_PTR glfGetModelViewProjectionMatrix(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if (Context->modelViewProjectionMatrix.recompute)
    {
        glsMATRIX_PTR temp;
        glsMATRIX product;

        if (Context->projectionMatrix->identity)
        {
            temp = Context->modelViewMatrix;
        }
        else if (Context->modelViewMatrix->identity)
        {
            temp = Context->projectionMatrix;
        }
        else
        {
            temp = &product;

            _MultiplyMatrix4x4(
                Context->projectionMatrix,
                Context->modelViewMatrix,
                temp
                );
        }

        glfConvertToVivanteMatrix(
            Context,
            temp,
            &Context->modelViewProjectionMatrix.matrix
            );

        Context->hashKey.hashModelViewProjectionIdentity =
            Context->modelViewProjectionMatrix.matrix.identity;

        Context->modelViewProjectionMatrix.recompute = GL_FALSE;
    }

    gcmFOOTER_ARG("result=0x%x", &Context->modelViewProjectionMatrix.matrix);
    return &Context->modelViewProjectionMatrix.matrix;
}


/*******************************************************************************
**
**  glfQueryMatrixState
**
**  Queries matrix state values.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Name
**          Specifies the symbolic name of the state to get.
**
**      Type
**          Data format.
**
**  OUTPUT:
**
**      Value
**          Points to the data.
**
*/

GLboolean glfQueryMatrixState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;
    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Name, Value, Type);

    switch (Name)
    {
    case GL_MATRIX_MODE:
        {
            static GLuint glMatrixMode[] =
            {
                GL_MODELVIEW,
                GL_PROJECTION,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_MATRIX_PALETTE_OES,
                GL_TEXTURE,
                GL_TEXTURE,
                GL_TEXTURE,
                GL_TEXTURE
            };

            glfGetFromEnum(
                glMatrixMode[Context->matrixMode],
                Value,
                Type
                );
        }
        break;

    case GL_MODELVIEW_MATRIX:
        glfGetFromMatrix(
            Context->modelViewMatrix,
            Value,
            Type
            );
        break;

    case GL_PROJECTION_MATRIX:
        glfGetFromMatrix(
            Context->projectionMatrix,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_MATRIX:
        glfGetFromMatrix(
            Context->textureMatrix,
            Value,
            Type
            );
        break;

    case GL_MAX_MODELVIEW_STACK_DEPTH:
        glfGetFromInt(
            glvMAX_STACK_NUM_MODELVIEW,
            Value,
            Type
            );
        break;

    case GL_MAX_PROJECTION_STACK_DEPTH:
        glfGetFromInt(
            glvMAX_STACK_NUM_PROJECTION,
            Value,
            Type
            );
        break;

    case GL_MAX_TEXTURE_STACK_DEPTH:
        glfGetFromInt(
            glvMAX_STACK_NUM_TEXTURES,
            Value,
            Type
            );
        break;

    case GL_MODELVIEW_STACK_DEPTH:
        glfGetFromInt(
            Context->matrixStackArray[glvMODEL_VIEW_MATRIX].index + 1,
            Value,
            Type
            );
        break;

    case GL_PROJECTION_STACK_DEPTH:
        glfGetFromInt(
            Context->matrixStackArray[glvPROJECTION_MATRIX].index + 1,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_STACK_DEPTH:
        {
            GLint stackIndex
                = glvTEXTURE_MATRIX_0
                + Context->texture.activeSamplerIndex;

            glfGetFromInt(
                Context->matrixStackArray[stackIndex].index + 1,
                Value,
                Type
                );
        }
        break;

    /* Supported under GL_OES_matrix_get extension. */
    case GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES:
        glfGetFromMatrix(
            Context->modelViewMatrix,
            Value,
            glvFLOAT
            );
        break;

    case GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES:
        glfGetFromMatrix(
            Context->projectionMatrix,
            Value,
            glvFLOAT
            );
        break;

    case GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES:
        glfGetFromMatrix(
            Context->textureMatrix,
            Value,
            glvFLOAT
            );
        break;

    case GL_CURRENT_PALETTE_MATRIX_OES:
        glfGetFromInt(
            Context->currentPalette,
            Value,
            glvINT
            );
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  glfMultiplyVector3ByMatrix3x3
**
**  Multiplies a 3-component (x, y, z) vector by a 3x3 matrix.
**
**  INPUT:
**
**      Vector
**          Points to the input vector.
**
**      Matrix
**          Points to the input matrix.
**
**  OUTPUT:
**
**      Result
**          Points to the resulting vector.
*/

void glfMultiplyVector3ByMatrix3x3(
    const glsVECTOR_PTR Vector,
    const glsMATRIX_PTR Matrix,
    glsVECTOR_PTR Result
    )
{
    gcmHEADER_ARG("Vector=0x%x Matrix=0x%04x Result=0x%x", Vector, Matrix, Result);
    /* Fast case. */
    if (Matrix->identity)
    {
        if (Result != Vector)
        {
            *Result = *Vector;
        }

        gcmFOOTER_NO();
        return;
    }

    /* Multiply if not identity. */
    switch (Vector->type)
    {
    case glvFIXED:

        switch (Matrix->type)
        {
        case glvFIXED:
            {
                GLint x, y;
                GLfixed result[3];

                for (y = 0; y < 3; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 3; x++)
                    {
                        result[y] += gcmXMultiply(
                            glmVECFIXED(Vector, x),
                            glmMATFIXED(Matrix, x, y)
                            );
                    }
                }

                glfSetVector3(Result, result, glvFIXED);
            }
            break;

#if !defined(COMMON_LITE)
        case glvFLOAT:
            {
                GLint x, y;
                GLfloat result[3];

                for (y = 0; y < 3; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 3; x++)
                    {
                        result[y]
                            += glmFIXED2FLOAT(glmVECFIXED(Vector, x))
                            *  glmMATFLOAT(Matrix, x, y);
                    }
                }

                glfSetVector3(Result, result, glvFLOAT);
            }
            break;
#endif

        default:
            gcmFATAL(
                "glfMultiplyVector3ByMatrix3x3: invalid matrix type %d",
                Matrix->type
                );
        }

        break;

#if !defined(COMMON_LITE)
    case glvFLOAT:

        switch (Matrix->type)
        {
        case glvFIXED:
            {
                GLint x, y;
                GLfloat result[3];

                for (y = 0; y < 3; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 3; x++)
                    {
                        result[y]
                            += glmVECFLOAT(Vector, x)
                            *  glmFIXED2FLOAT(glmMATFIXED(Matrix, x, y));
                    }
                }

                glfSetVector3(Result, result, glvFLOAT);
            }
            break;

        case glvFLOAT:
            {
                GLint x, y;
                GLfloat result[3];

                for (y = 0; y < 3; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 3; x++)
                    {
                        result[y]
                            += glmVECFLOAT(Vector, x)
                            *  glmMATFLOAT(Matrix, x, y);
                    }
                }

                glfSetVector3(Result, result, glvFLOAT);
            }
            break;

        default:
            gcmFATAL(
                "glfMultiplyVector3ByMatrix3x3: invalid matrix type %d",
                Matrix->type
                );
        }

        break;
#endif

    default:
        gcmFATAL(
            "glfMultiplyVector3ByMatrix3x3: invalid vector type %d",
            Vector->type
            );
    }

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  glfMultiplyVector4ByMatrix4x4
**
**  Multiplies a 4-component (x, y, z, w) vector by a 4x4 matrix.
**
**  INPUT:
**
**      Matrix
**          Points to a matrix.
**
**      Vector
**          Points to a column vector.
**
**  OUTPUT:
**
**       Result
**          A vector, product of multiplication.
*/

void glfMultiplyVector4ByMatrix4x4(
    const glsVECTOR_PTR Vector,
    const glsMATRIX_PTR Matrix,
    glsVECTOR_PTR Result
    )
{
    gcmHEADER_ARG("Vector=0x%x Matrix=0x%04x Result=0x%x", Vector, Matrix, Result);
    /* Fast case. */
    if (Matrix->identity)
    {
        if (Result != Vector)
        {
            *Result = *Vector;
        }

        gcmFOOTER_NO();
        return;
    }

    /* Multiply if not identity. */
    switch (Vector->type)
    {
    case glvFIXED:

        switch (Matrix->type)
        {
        case glvFIXED:
            {
                GLint x, y;
                GLfixed result[4];

                for (y = 0; y < 4; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 4; x++)
                    {
                        result[y] += gcmXMultiply(
                            glmVECFIXED(Vector, x),
                            glmMATFIXED(Matrix, x, y)
                            );
                    }
                }

                glfSetVector4(Result, result, glvFIXED);
            }
            break;

#if !defined(COMMON_LITE)
        case glvFLOAT:
            {
                GLint x, y;
                GLfloat result[4];

                for (y = 0; y < 4; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 4; x++)
                    {
                        result[y]
                            += glmFIXED2FLOAT(glmVECFIXED(Vector, x))
                            *  glmMATFLOAT(Matrix, x, y);
                    }
                }

                glfSetVector4(Result, result, glvFLOAT);
            }
            break;
#endif

        default:
            gcmFATAL(
                "glfMultiplyVector4ByMatrix4x4: invalid matrix type %d",
                Matrix->type
                );
        }

        break;

#if !defined(COMMON_LITE)
    case glvFLOAT:

        switch (Matrix->type)
        {
        case glvFIXED:
            {
                GLint x, y;
                GLfloat result[4];

                for (y = 0; y < 4; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 4; x++)
                    {
                        result[y]
                            += glmVECFLOAT(Vector, x)
                            *  glmFIXED2FLOAT(glmMATFIXED(Matrix, x, y));
                    }
                }

                glfSetVector4(Result, result, glvFLOAT);
            }
            break;

        case glvFLOAT:
            {
                GLint x, y;
                GLfloat result[4];

                for (y = 0; y < 4; y++)
                {
                    result[y] = 0;

                    for (x = 0; x < 4; x++)
                    {
                        result[y]
                            += glmVECFLOAT(Vector, x)
                            *  glmMATFLOAT(Matrix, x, y);
                    }
                }

                glfSetVector4(Result, result, glvFLOAT);
            }
            break;

        default:
            gcmFATAL(
                "glfMultiplyVector4ByMatrix4x4: invalid matrix type %d",
                Matrix->type
                );
        }

        break;
#endif

    default:
        gcmFATAL(
            "glfMultiplyVector4ByMatrix4x4: invalid vector type %d",
            Vector->type
            );
    }

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  glfSinX / glfCosX
**
**  Sine and Cosine fixed point functions.
**
**  INPUT:
**
**      Angle
**          Angle in radians.
**
**  OUTPUT:
**
**      Result of the function.
*/

static const GLfixed sinx_table[1024] =
{
    0x000000, 0x000097, 0x0000fb, 0x000160, 0x0001c4, 0x000229, 0x00028d, 0x0002f2,
    0x000356, 0x0003bb, 0x000420, 0x000484, 0x0004e9, 0x00054d, 0x0005b2, 0x000616,
    0x00067b, 0x0006df, 0x000744, 0x0007a8, 0x00080d, 0x000871, 0x0008d5, 0x00093a,
    0x00099e, 0x000a03, 0x000a67, 0x000acc, 0x000b30, 0x000b95, 0x000bf9, 0x000c5d,
    0x000cc2, 0x000d56, 0x000d66, 0x000def, 0x000e53, 0x000eb8, 0x000f1c, 0x000f81,
    0x000fe5, 0x001049, 0x0010ae, 0x001112, 0x001176, 0x0011da, 0x00123f, 0x0012a3,
    0x001307, 0x00136c, 0x0013d0, 0x001434, 0x001498, 0x0014fc, 0x001561, 0x0015c5,
    0x001629, 0x00168d, 0x0016f1, 0x001755, 0x0017b9, 0x00181d, 0x001882, 0x0018e6,
    0x00194a, 0x0019ae, 0x001a12, 0x001a76, 0x001ada, 0x001b3e, 0x001ba2, 0x001c06,
    0x001c69, 0x001ccd, 0x001d31, 0x001d95, 0x001df9, 0x001e5d, 0x001ec1, 0x001f24,
    0x001f88, 0x001fec, 0x002050, 0x0020b3, 0x002117, 0x00217b, 0x0021de, 0x002242,
    0x0022a6, 0x002309, 0x00236d, 0x0023d0, 0x002434, 0x002497, 0x0024fb, 0x00255e,
    0x0025c2, 0x002625, 0x002689, 0x0026ec, 0x00274f, 0x0027b3, 0x002816, 0x002879,
    0x0028dd, 0x002940, 0x0029a3, 0x002a06, 0x002a69, 0x002acc, 0x002b30, 0x002b93,
    0x002bf6, 0x002c59, 0x002cbc, 0x002d1f, 0x002d82, 0x002de5, 0x002e47, 0x002eaa,
    0x002f0d, 0x002f70, 0x002fd3, 0x003035, 0x003098, 0x0030fb, 0x00315e, 0x0031c0,
    0x003223, 0x003285, 0x0032e8, 0x00334a, 0x0033ad, 0x00340f, 0x003472, 0x0034d4,
    0x003536, 0x003599, 0x0035fb, 0x00365d, 0x0036c0, 0x003722, 0x003784, 0x0037e6,
    0x003848, 0x0038aa, 0x00390c, 0x00396e, 0x0039d0, 0x003a32, 0x003a94, 0x003af6,
    0x003b58, 0x003bb9, 0x003c1b, 0x003c7d, 0x003cde, 0x003d40, 0x003da2, 0x003e03,
    0x003e65, 0x003ec6, 0x003f28, 0x003f89, 0x003fea, 0x00404c, 0x0040ad, 0x00410e,
    0x00416f, 0x0041d1, 0x004232, 0x004293, 0x0042f4, 0x004355, 0x0043b6, 0x004417,
    0x004478, 0x0044d9, 0x004539, 0x00459a, 0x0045fb, 0x00465c, 0x0046bc, 0x00471d,
    0x00477d, 0x0047de, 0x00483e, 0x00489f, 0x0048ff, 0x00495f, 0x0049c0, 0x004a20,
    0x004a80, 0x004ae0, 0x004b40, 0x004ba1, 0x004c01, 0x004c61, 0x004cc0, 0x004d20,
    0x004d80, 0x004de0, 0x004e40, 0x004e9f, 0x004eff, 0x004f5f, 0x004fbe, 0x00501e,
    0x00507d, 0x0050dd, 0x00513c, 0x00519b, 0x0051fb, 0x00525a, 0x0052b9, 0x005318,
    0x005377, 0x0053d6, 0x005435, 0x005494, 0x0054f3, 0x005552, 0x0055b0, 0x00560f,
    0x00566e, 0x0056cc, 0x00572b, 0x005789, 0x0057e8, 0x005846, 0x0058a5, 0x005903,
    0x005961, 0x0059bf, 0x005a1d, 0x005a7b, 0x005ad9, 0x005b37, 0x005b95, 0x005bf3,
    0x005c51, 0x005caf, 0x005d0c, 0x005d6a, 0x005dc8, 0x005e25, 0x005e83, 0x005ee0,
    0x005f3d, 0x005f9b, 0x005ff8, 0x006055, 0x0060b2, 0x00610f, 0x00616c, 0x0061c9,
    0x006226, 0x006283, 0x0062e0, 0x00633c, 0x006399, 0x0063f5, 0x006452, 0x0064ae,
    0x00650b, 0x006567, 0x0065c3, 0x006620, 0x00667c, 0x0066d8, 0x006734, 0x006790,
    0x0067ec, 0x006848, 0x0068a3, 0x0068ff, 0x00695b, 0x0069b6, 0x006a12, 0x006a6d,
    0x006ac9, 0x006b24, 0x006b7f, 0x006bdb, 0x006c36, 0x006c91, 0x006cec, 0x006d47,
    0x006da2, 0x006dfc, 0x006e57, 0x006eb2, 0x006f0d, 0x006f67, 0x006fc2, 0x00701c,
    0x007076, 0x0070d1, 0x00712b, 0x007185, 0x0071df, 0x007239, 0x007293, 0x0072ed,
    0x007347, 0x0073a0, 0x0073fa, 0x007454, 0x0074ad, 0x007507, 0x007560, 0x0075b9,
    0x007612, 0x00766c, 0x0076c5, 0x00771e, 0x007777, 0x0077d0, 0x007828, 0x007881,
    0x0078da, 0x007932, 0x00798b, 0x0079e3, 0x007a3c, 0x007a94, 0x007aec, 0x007b44,
    0x007b9c, 0x007bf4, 0x007c4c, 0x007ca4, 0x007cfc, 0x007d54, 0x007dab, 0x007e03,
    0x007e5a, 0x007eb2, 0x007f09, 0x007f60, 0x007fb7, 0x00800f, 0x008066, 0x0080bc,
    0x008113, 0x00816a, 0x0081c1, 0x008217, 0x00826e, 0x0082c4, 0x00831b, 0x008371,
    0x0083c7, 0x00841d, 0x008474, 0x0084ca, 0x00851f, 0x008575, 0x0085cb, 0x008621,
    0x008676, 0x0086cc, 0x008721, 0x008777, 0x0087cc, 0x008821, 0x008876, 0x0088cb,
    0x008920, 0x008975, 0x0089ca, 0x008a1e, 0x008a73, 0x008ac7, 0x008b1c, 0x008b70,
    0x008bc5, 0x008c19, 0x008c6d, 0x008cc1, 0x008d15, 0x008d69, 0x008dbc, 0x008e10,
    0x008e64, 0x008eb7, 0x008f0b, 0x008f5e, 0x008fb1, 0x009004, 0x009057, 0x0090aa,
    0x0090fd, 0x009150, 0x0091a3, 0x0091f5, 0x009248, 0x00929a, 0x0092ed, 0x00933f,
    0x009391, 0x0093e3, 0x009435, 0x009487, 0x0094d9, 0x00952b, 0x00957d, 0x0095ce,
    0x009620, 0x009671, 0x0096c2, 0x009713, 0x009765, 0x0097b6, 0x009807, 0x009857,
    0x0098a8, 0x0098f9, 0x009949, 0x00999a, 0x0099ea, 0x009a3a, 0x009a8b, 0x009adb,
    0x009b2b, 0x009b7b, 0x009bca, 0x009c1a, 0x009c6a, 0x009cb9, 0x009d09, 0x009d58,
    0x009da7, 0x009df7, 0x009e46, 0x009e95, 0x009ee3, 0x009f32, 0x009f81, 0x009fd0,
    0x00a01e, 0x00a06c, 0x00a0bb, 0x00a109, 0x00a157, 0x00a1a5, 0x00a1f3, 0x00a241,
    0x00a28e, 0x00a2dc, 0x00a32a, 0x00a377, 0x00a3c4, 0x00a412, 0x00a45f, 0x00a4ac,
    0x00a4f9, 0x00a545, 0x00a592, 0x00a5df, 0x00a62b, 0x00a678, 0x00a6c4, 0x00a710,
    0x00a75c, 0x00a7a8, 0x00a7f4, 0x00a840, 0x00a88c, 0x00a8d7, 0x00a923, 0x00a96e,
    0x00a9ba, 0x00aa05, 0x00aa50, 0x00aa9b, 0x00aae6, 0x00ab31, 0x00ab7b, 0x00abc6,
    0x00ac11, 0x00ac5b, 0x00aca5, 0x00acef, 0x00ad39, 0x00ad83, 0x00adcd, 0x00ae17,
    0x00ae61, 0x00aeaa, 0x00aef4, 0x00af3d, 0x00af86, 0x00afcf, 0x00b018, 0x00b061,
    0x00b0aa, 0x00b0f3, 0x00b13b, 0x00b184, 0x00b1cc, 0x00b215, 0x00b25d, 0x00b2a5,
    0x00b2ed, 0x00b335, 0x00b37c, 0x00b3c4, 0x00b40b, 0x00b453, 0x00b49a, 0x00b4e1,
    0x00b528, 0x00b56f, 0x00b5b6, 0x00b5fd, 0x00b644, 0x00b68a, 0x00b6d1, 0x00b717,
    0x00b75d, 0x00b7a3, 0x00b7e9, 0x00b82f, 0x00b875, 0x00b8bb, 0x00b900, 0x00b946,
    0x00b98b, 0x00b9d0, 0x00ba15, 0x00ba5a, 0x00ba9f, 0x00bae4, 0x00bb28, 0x00bb6d,
    0x00bbb1, 0x00bbf6, 0x00bc3a, 0x00bc7e, 0x00bcc2, 0x00bd06, 0x00bd4a, 0x00bd8d,
    0x00bdd1, 0x00be14, 0x00be57, 0x00be9b, 0x00bede, 0x00bf21, 0x00bf63, 0x00bfa6,
    0x00bfe9, 0x00c02b, 0x00c06e, 0x00c0b0, 0x00c0f2, 0x00c134, 0x00c176, 0x00c1b8,
    0x00c1f9, 0x00c23b, 0x00c27c, 0x00c2be, 0x00c2ff, 0x00c340, 0x00c381, 0x00c3c2,
    0x00c402, 0x00c443, 0x00c483, 0x00c4c4, 0x00c504, 0x00c544, 0x00c584, 0x00c5c4,
    0x00c604, 0x00c644, 0x00c683, 0x00c6c2, 0x00c702, 0x00c741, 0x00c780, 0x00c7bf,
    0x00c7fe, 0x00c83c, 0x00c87b, 0x00c8ba, 0x00c8f8, 0x00c936, 0x00c974, 0x00c9b2,
    0x00c9f0, 0x00ca2e, 0x00ca6b, 0x00caa9, 0x00cae6, 0x00cb23, 0x00cb61, 0x00cb9e,
    0x00cbda, 0x00cc17, 0x00cc54, 0x00cc90, 0x00cccd, 0x00cd09, 0x00cd45, 0x00cd81,
    0x00cdbd, 0x00cdf9, 0x00ce34, 0x00ce70, 0x00ceab, 0x00cee7, 0x00cf22, 0x00cf5d,
    0x00cf98, 0x00cfd2, 0x00d00d, 0x00d047, 0x00d082, 0x00d0bc, 0x00d0f6, 0x00d130,
    0x00d16a, 0x00d1a4, 0x00d1de, 0x00d217, 0x00d250, 0x00d28a, 0x00d2c3, 0x00d2fc,
    0x00d335, 0x00d36d, 0x00d3a6, 0x00d3df, 0x00d417, 0x00d44f, 0x00d487, 0x00d4bf,
    0x00d4f7, 0x00d52f, 0x00d566, 0x00d59e, 0x00d5d5, 0x00d60c, 0x00d644, 0x00d67a,
    0x00d6b1, 0x00d6e8, 0x00d71f, 0x00d755, 0x00d78b, 0x00d7c1, 0x00d7f8, 0x00d82d,
    0x00d863, 0x00d899, 0x00d8ce, 0x00d904, 0x00d939, 0x00d96e, 0x00d9a3, 0x00d9d8,
    0x00da0d, 0x00da41, 0x00da76, 0x00daaa, 0x00dade, 0x00db12, 0x00db46, 0x00db7a,
    0x00dbae, 0x00dbe1, 0x00dc15, 0x00dc48, 0x00dc7b, 0x00dcae, 0x00dce1, 0x00dd14,
    0x00dd47, 0x00dd79, 0x00ddab, 0x00ddde, 0x00de10, 0x00de42, 0x00de74, 0x00dea5,
    0x00ded7, 0x00df08, 0x00df39, 0x00df6b, 0x00df9c, 0x00dfcd, 0x00dffd, 0x00e02e,
    0x00e05e, 0x00e08f, 0x00e0bf, 0x00e0ef, 0x00e11f, 0x00e14f, 0x00e17e, 0x00e1ae,
    0x00e1dd, 0x00e20d, 0x00e23c, 0x00e26b, 0x00e299, 0x00e2c8, 0x00e2f7, 0x00e325,
    0x00e353, 0x00e382, 0x00e3b0, 0x00e3de, 0x00e40b, 0x00e439, 0x00e466, 0x00e494,
    0x00e4c1, 0x00e4ee, 0x00e51b, 0x00e548, 0x00e574, 0x00e5a1, 0x00e5cd, 0x00e5f9,
    0x00e626, 0x00e652, 0x00e67d, 0x00e6a9, 0x00e6d5, 0x00e700, 0x00e72b, 0x00e756,
    0x00e781, 0x00e7ac, 0x00e7d7, 0x00e801, 0x00e82c, 0x00e856, 0x00e880, 0x00e8aa,
    0x00e8d4, 0x00e8fe, 0x00e927, 0x00e951, 0x00e97a, 0x00e9a3, 0x00e9cc, 0x00e9f5,
    0x00ea1e, 0x00ea47, 0x00ea6f, 0x00ea97, 0x00eac0, 0x00eae8, 0x00eb0f, 0x00eb37,
    0x00eb5f, 0x00eb86, 0x00ebae, 0x00ebd5, 0x00ebfc, 0x00ec23, 0x00ec4a, 0x00ec70,
    0x00ec97, 0x00ecbd, 0x00ece3, 0x00ed09, 0x00ed2f, 0x00ed55, 0x00ed7a, 0x00eda0,
    0x00edc5, 0x00edea, 0x00ee0f, 0x00ee34, 0x00ee59, 0x00ee7e, 0x00eea2, 0x00eec7,
    0x00eeeb, 0x00ef0f, 0x00ef33, 0x00ef56, 0x00ef7a, 0x00ef9d, 0x00efc1, 0x00efe4,
    0x00f007, 0x00f02a, 0x00f04d, 0x00f06f, 0x00f092, 0x00f0b4, 0x00f0d6, 0x00f0f8,
    0x00f11a, 0x00f13c, 0x00f15d, 0x00f17f, 0x00f1a0, 0x00f1c1, 0x00f1e2, 0x00f203,
    0x00f224, 0x00f244, 0x00f265, 0x00f285, 0x00f2a5, 0x00f2c5, 0x00f2e5, 0x00f304,
    0x00f324, 0x00f343, 0x00f363, 0x00f382, 0x00f3a1, 0x00f3c0, 0x00f3de, 0x00f3fd,
    0x00f41b, 0x00f439, 0x00f457, 0x00f475, 0x00f493, 0x00f4b1, 0x00f4ce, 0x00f4eb,
    0x00f509, 0x00f526, 0x00f543, 0x00f55f, 0x00f57c, 0x00f598, 0x00f5b5, 0x00f5d1,
    0x00f5ed, 0x00f609, 0x00f624, 0x00f640, 0x00f65b, 0x00f677, 0x00f692, 0x00f6ad,
    0x00f6c7, 0x00f6e2, 0x00f6fd, 0x00f717, 0x00f731, 0x00f74b, 0x00f765, 0x00f77f,
    0x00f799, 0x00f7b2, 0x00f7cb, 0x00f7e5, 0x00f7fe, 0x00f816, 0x00f82f, 0x00f848,
    0x00f860, 0x00f878, 0x00f891, 0x00f8a9, 0x00f8c0, 0x00f8d8, 0x00f8f0, 0x00f907,
    0x00f91e, 0x00f935, 0x00f94c, 0x00f963, 0x00f97a, 0x00f990, 0x00f9a6, 0x00f9bd,
    0x00f9d3, 0x00f9e8, 0x00f9fe, 0x00fa14, 0x00fa29, 0x00fa3e, 0x00fa54, 0x00fa69,
    0x00fa7d, 0x00fa92, 0x00faa7, 0x00fabb, 0x00facf, 0x00fae3, 0x00faf7, 0x00fb0b,
    0x00fb1f, 0x00fb32, 0x00fb45, 0x00fb58, 0x00fb6b, 0x00fb7e, 0x00fb91, 0x00fba4,
    0x00fbb6, 0x00fbc8, 0x00fbda, 0x00fbec, 0x00fbfe, 0x00fc10, 0x00fc21, 0x00fc33,
    0x00fc44, 0x00fc55, 0x00fc66, 0x00fc76, 0x00fc87, 0x00fc97, 0x00fca8, 0x00fcb8,
    0x00fcc8, 0x00fcd8, 0x00fce7, 0x00fcf7, 0x00fd06, 0x00fd15, 0x00fd24, 0x00fd33,
    0x00fd42, 0x00fd51, 0x00fd5f, 0x00fd6d, 0x00fd7c, 0x00fd89, 0x00fd97, 0x00fda5,
    0x00fdb3, 0x00fdc0, 0x00fdcd, 0x00fdda, 0x00fde7, 0x00fdf4, 0x00fe01, 0x00fe0d,
    0x00fe19, 0x00fe25, 0x00fe31, 0x00fe3d, 0x00fe49, 0x00fe55, 0x00fe60, 0x00fe6b,
    0x00fe76, 0x00fe81, 0x00fe8c, 0x00fe97, 0x00fea1, 0x00feab, 0x00feb5, 0x00febf,
    0x00fec9, 0x00fed3, 0x00fedd, 0x00fee6, 0x00feef, 0x00fef8, 0x00ff01, 0x00ff0a,
    0x00ff13, 0x00ff1b, 0x00ff23, 0x00ff2c, 0x00ff34, 0x00ff3b, 0x00ff43, 0x00ff4b,
    0x00ff52, 0x00ff59, 0x00ff60, 0x00ff67, 0x00ff6e, 0x00ff75, 0x00ff7b, 0x00ff82,
    0x00ff88, 0x00ff8e, 0x00ff94, 0x00ff99, 0x00ff9f, 0x00ffa4, 0x00ffa7, 0x00ffaf,
    0x00ffb4, 0x00ffb8, 0x00ffbd, 0x00ffc1, 0x00ffc6, 0x00ffca, 0x00ffce, 0x00ffd2,
    0x00ffd5, 0x00ffd9, 0x00ffdc, 0x00ffe0, 0x00ffe3, 0x00ffe6, 0x00ffe8, 0x00ffeb,
    0x00ffed, 0x00fff0, 0x00fff2, 0x00fff4, 0x00fff6, 0x00fff7, 0x00fff9, 0x00fffa,
    0x00fffc, 0x00fffd, 0x00fffe, 0x00fffe, 0x00ffff, 0x010000, 0x010000, 0x010000
};

GLfixed glfSinX(
     GLfixed Angle
     )
{
    GLfixed a;
    GLfixed result;
    gcmHEADER_ARG("Angle=0x%08x", Angle);

    /* Keep adding 2 * pi until angle is positive. */
    for (a = Angle; a < 0; a += glvFIXEDPITIMES2) ;

    /* Divide angle by 2 * pi. */
    a = gcmXDivide(a, glvFIXEDPITIMES2);

    /* Drop 4 bits of significance. */
    a >>= 4;

    /* Lookup sin value. */
    switch (a & 0xC00)
    {
    case 0x400:
        result = sinx_table[0x3FF - (a & 0x3FF)];
        break;

    case 0x800:
        result= -sinx_table[a & 0x3FF];
        break;

    case 0xC00:
        result = -sinx_table[0x3FF - (a & 0x3FF)];
        break;

    default:
        result = sinx_table[a & 0x3FF];
        break;
    }
    gcmFOOTER_ARG("result=0x%08x", result);
    return result;
}

GLfixed glfCosX(
     GLfixed Angle
     )
{
    GLfixed a;
    GLfixed result;
    gcmHEADER_ARG("Angle=0x%08x", Angle);

    /* Keep adding 2 * pi until angle is positive. */
    for (a = Angle; a < 0; a += glvFIXEDPITIMES2) ;

    /* Divide angle by 2 * pi. */
    a = gcmXDivide(a, glvFIXEDPITIMES2);

    /* Drop 4 bits of significance. */
    a >>= 4;

    /* Lookup cos value. */
    switch (a & 0xC00)
    {
    case 0x000:
        result = sinx_table[0x3FF - (a & 0x3FF)];
        break;

    case 0x400:
        result = -sinx_table[a & 0x3FF];
        break;

    case 0x800:
        result = -sinx_table[0x3FF - (a & 0x3FF)];
        break;

    default:
        result = sinx_table[a & 0x3FF];
        break;
    }
    gcmFOOTER_ARG("result=0x%08x", result);
    return result;
}


/*******************************************************************************
**
**  glfRSQX
**
**  Reciprocal square root in fixed point.
**
**  INPUT:
**
**      X
**          Input value.
**
**  OUTPUT:
**
**      1 / sqrt(X).
*/
GLfixed glfRSQX(
    GLfixed X
    )
{
    static const GLushort rsqrtx_table[8] =
    {
        0x6A0A, 0x5555, 0x43D1, 0x34BF, 0x279A, 0x1C02, 0x11AD, 0x0865
    };

    GLfixed r;
    int exp, i;

    gcmHEADER_ARG("X=0x%08x", X);

    gcmASSERT(X >= 0);

    if (X == gcvONE_X)
    {
        gcmFOOTER_ARG("0x%x", gcvONE_X);
        return gcvONE_X;
    }

    r   = X;
    exp = 31;

    if (r & 0xFFFF0000)
    {
        exp -= 16;
        r >>= 16;
    }

    if (r & 0xFF00)
    {
        exp -= 8;
        r >>= 8;
    }

    if (r & 0xF0)
    {
        exp -= 4;
        r >>= 4;
    }

    if (r & 0xC)
    {
        exp -= 2;
        r >>= 2;
    }

    if (r & 0x2)
    {
        exp -= 1;
    }

    if (exp > 28)
    {
        static const GLfixed low_value_result[] =
        {
            0x7FFFFFFF, 0x01000000, 0x00B504F3, 0x0093CD3A,
            0x00800000, 0x00727C97, 0x006882F5, 0x0060C247
        };

        gcmFOOTER_ARG("0x%x", low_value_result[X & 7]);
        return low_value_result[X & 7];
    }

    r    = gcvONE_X + rsqrtx_table[(X >> (28 - exp)) & 0x7];
    exp -= 16;

    if (exp <= 0)
    {
        r >>= -exp >> 1;
    }
    else
    {
        r <<= (exp >> 1) + (exp & 1);
    }

    if (exp & 1)
    {
        r = gcmXMultiply(r, rsqrtx_table[0]);
    }

    for (i = 0; i < 3; i++)
    {
        r = gcmXMultiply(
            (r >> 1),
            (3 << 16) - gcmXMultiply(gcmXMultiply(r, r), X)
            );
    }

    gcmFOOTER_ARG("0x%x", r);
    return r;
}


/*******************************************************************************
**
**  _Norm3
**
**  Vector normalization.
**
**  INPUT:
**
**      X, Y, Z
**          Vector.
**
**  OUTPUT:
**
**      Normal
**          Normalized vector.
*/

static void _Norm3X(
    GLfixed X,
    GLfixed Y,
    GLfixed Z,
    glsVECTOR_PTR Normal
    )
{
    /* Compute normal. */
    GLfixed sum
        = gcmXMultiply(X, X)
        + gcmXMultiply(Y, Y)
        + gcmXMultiply(Z, Z);

    GLfixed norm = glfRSQX(sum);

    gcmHEADER_ARG("X=0x%08x Y=0x%08x Z=0x%08x Normal=0x%x", X, Y, Z, Normal);

    /* Multiply vector by normal. */
    X = gcmXMultiply(X, norm);
    Y = gcmXMultiply(Y, norm);
    Z = gcmXMultiply(Z, norm);

    /* Set the result. */
    glfSetFixedVector4(Normal, X, Y, Z, 0);
    gcmFOOTER_NO();
}

#if !defined(COMMON_LITE)
static void _Norm3F(
    GLfloat X,
    GLfloat Y,
    GLfloat Z,
    glsVECTOR_PTR Normal
    )
{
    /* Compute normal. */
    GLfloat sum  = X * X + Y * Y + Z * Z;
    GLfloat norm = 1.0f / gcoMATH_SquareRoot(sum);

    gcmHEADER_ARG("X=0x%08x Y=0x%08x Z=0x%08x Normal=0x%x", X, Y, Z, Normal);
    /* Multiply vector by normal. */
    X = X * norm;
    Y = Y * norm;
    Z = Z * norm;

    /* Set the result. */
    glfSetFloatVector4(Normal, X, Y, Z, 0);
    gcmFOOTER_NO();
}
#endif


/******************************************************************************\
************************* OpenGL Matrix Management Code ************************
\******************************************************************************/

/*******************************************************************************
**
**  glMatrixMode
**
**  glMatrixMode sets the current matrix mode. Mode can assume one of four
**  values:
**
**     GL_MODELVIEW
**        Applies subsequent matrix operations to the modelview matrix stack.
**
**     GL_PROJECTION
**        Applies subsequent matrix operations to the projection matrix stack.
**
**     GL_TEXTURE
**        Applies subsequent matrix operations to the texture matrix stack.
**
**     GL_MATRIX_PALETTE_OES
**        Enables the matrix palette stack extension, and applies subsequent
**        matrix operations to the matrix palette stack.
**
**  INPUT:
**
**      Mode
**          Specifies which matrix stack is the target for subsequent matrix
**          operations. The initial value is GL_MODELVIEW.
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_MATRIX

GL_API void GL_APIENTRY glMatrixMode(
    GLenum Mode
    )
{
    glmENTER1(glmARGENUM, Mode)
    {
		gcmDUMP_API("${ES11 glMatrixMode 0x%08X}", Mode);

        glmPROFILE(context, GLES1_MATRIXMODE, 0);
        glmERROR(glfSetMatrixMode(context, Mode));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glLoadIdentity
**
**  glLoadIdentity replaces the current matrix with the identity matrix. It is
**  semantically equivalent to calling glLoadMatrix with the identity matrix:
**
**      1   0   0   0
**      0   1   0   0
**      0   0   1   0
**      0   0   0   1
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glLoadIdentity(
    void
    )
{
    glmENTER()
    {
		gcmDUMP_API("${ES11 glLoadIdentity}");

        glmPROFILE(context, GLES1_LOADIDENTITY, 0);
        /* Initialize identity matrix. */
        _LoadIdentityMatrix(context->currentMatrix, glvFRACTYPEENUM);

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glLoadMatrix
**
**  glLoadMatrix replaces the current matrix with the one whose elements are
**  specified by Matrix. The current matrix is the projection matrix, modelview
**  matrix, or texture matrix, depending on the current matrix mode
**  (see glMatrixMode).
**
**  INPUT:
**
**      Matrix
**          Specifies a pointer to 16 consecutive values, which are used as the
**          elements of a 4 ? 4 column-major matrix.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glLoadMatrixx(
    const GLfixed* Matrix
    )
{
    glmENTER1(glmARGPTR, Matrix)
    {
		gcmDUMP_API("${ES11 glLoadMatrixx (0x%08X)", Matrix);
		gcmDUMP_API_ARRAY(Matrix, 16);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_LOADMATRIXX, 0);

        /* Load the matrix. */
        if(_LoadMatrix(context->currentMatrix, glvFIXED, Matrix))
        {
            /* Notify dependents of the change. */
            (*context->currentStack->dataChanged) (context);
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glLoadMatrixxOES(
    const GLfixed* Matrix
    )
{
    glmENTER1(glmARGPTR, Matrix)
    {
		gcmDUMP_API("${ES11 glLoadMatrixxOES (0x%08X)", Matrix);
		gcmDUMP_API_ARRAY(Matrix, 16);
		gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_LOADMATRIXX, 0);

        /* Load the matrix. */
        if(_LoadMatrix(context->currentMatrix, glvFIXED, Matrix))
        {
            /* Notify dependents of the change. */
            (*context->currentStack->dataChanged) (context);
        }
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glLoadMatrixf(
    const GLfloat* Matrix
    )
{
    glmENTER1(glmARGPTR, Matrix)
    {
		gcmDUMP_API("${ES11 glLoadMatrixf (0x%08X)", Matrix);
		gcmDUMP_API_ARRAY(Matrix, 16);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_LOADMATRIXF, 0);

        /* Load the matrix. */
        if(_LoadMatrix(context->currentMatrix, glvFLOAT, Matrix))
        {
            /* Notify dependents of the change. */
            (*context->currentStack->dataChanged) (context);
        }
    }
    glmLEAVE();
}
#endif

static void _Orthox(
    GLfixed Left,
    GLfixed Right,
    GLfixed Bottom,
    GLfixed Top,
    GLfixed zNear,
    GLfixed zFar
    )
{
    glmENTER6(glmARGFIXED, Left, glmARGFIXED, Right, glmARGFIXED, Bottom,
              glmARGFIXED, Top, glmARGFIXED, zNear, glmARGFIXED, zFar)
    {
        glsMATRIX ortho, result;

		glmPROFILE(context, GLES1_ORTHOX, 0);

        /* Verify arguments. */
        if ((Left == Right) ||
            (Bottom == Top) ||
            (zNear == zFar))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Fill in ortho matrix. */
        gcoOS_ZeroMemory(&ortho, sizeof(ortho));
        glmMATFIXED(&ortho, 0, 0) = gcmXDivide(gcvTWO_X, Right - Left);
        glmMATFIXED(&ortho, 1, 1) = gcmXDivide(gcvTWO_X, Top   - Bottom);
        glmMATFIXED(&ortho, 2, 2) = gcmXDivide(gcvTWO_X, zNear - zFar);
        glmMATFIXED(&ortho, 3, 0) = gcmXDivide(Right + Left,   Left   - Right);
        glmMATFIXED(&ortho, 3, 1) = gcmXDivide(Top   + Bottom, Bottom - Top);
        glmMATFIXED(&ortho, 3, 2) = gcmXDivide(zFar  + zNear,  zNear  - zFar);
        glmMATFIXED(&ortho, 3, 3) = gcvONE_X;
        ortho.type = glvFIXED;

        /* Multipy the current matrix by the ortho matrix. */
        if(_MultiplyMatrix4x4(context->currentMatrix, &ortho, &result))
        {
            /* Replace the current matrix. */
            *context->currentMatrix = result;

            /* Notify dependents of the change. */
            (*context->currentStack->dataChanged) (context);
        }
    }
    glmLEAVE();
}



/*******************************************************************************
**
**  glOrtho
**
**  glOrtho describes a transformation that produces a parallel projection.
**  The current matrix (see glMatrixMode) is multiplied by this matrix and
**  the result replaces the current matrix.
**
**  INPUT:
**
**      Left
**      Right
**          Specify the coordinates for the left and right vertical
**          clipping planes.
**
**      Bottom
**      Top
**          Specify the coordinates for the bottom and top horizontal
**          clipping planes.
**
**      zNear
**      zFar
**          Specify the distances to the nearer and farther depth clipping
**          planes. These values are negative if the plane is to be behind
**          the viewer.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glOrthox(
    GLfixed Left,
    GLfixed Right,
    GLfixed Bottom,
    GLfixed Top,
    GLfixed zNear,
    GLfixed zFar
    )
{
	gcmDUMP_API("${ES11 glOrthox 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		Left, Right, Bottom, Top, zNear, zFar);

    _Orthox(Left, Right, Bottom, Top, zNear, zFar);
}

GL_API void GL_APIENTRY glOrthoxOES(
    GLfixed Left,
    GLfixed Right,
    GLfixed Bottom,
    GLfixed Top,
    GLfixed zNear,
    GLfixed zFar
    )
{
	gcmDUMP_API("${ES11 glOrthoxOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		Left, Right, Bottom, Top, zNear, zFar);

    _Orthox(Left, Right, Bottom, Top, zNear, zFar);
}

#ifndef COMMON_LITE
static void _Orthof(
    GLfloat Left,
    GLfloat Right,
    GLfloat Bottom,
    GLfloat Top,
    GLfloat zNear,
    GLfloat zFar
    )
{
    glmENTER6(glmARGFLOAT, Left, glmARGFLOAT, Right, glmARGFLOAT, Bottom,
              glmARGFLOAT, Top, glmARGFLOAT, zNear, glmARGFLOAT, zFar)
    {
        glsMATRIX ortho, result;

	    glmPROFILE(context, GLES1_ORTHOF, 0);

        /* Verify arguments. */
        if ((Left == Right) ||
            (Bottom == Top) ||
            (zNear == zFar))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Fill in ortho matrix. */
        gcoOS_ZeroMemory(&ortho, sizeof(ortho));
        glmMATFLOAT(&ortho, 0, 0) =  2.0f          / (Right  - Left);
        glmMATFLOAT(&ortho, 1, 1) =  2.0f          / (Top    - Bottom);
        glmMATFLOAT(&ortho, 2, 2) =  2.0f          / (zNear  - zFar);
        glmMATFLOAT(&ortho, 3, 0) = (Right + Left) / (Left   - Right);
        glmMATFLOAT(&ortho, 3, 1) = (Top + Bottom) / (Bottom - Top);
        glmMATFLOAT(&ortho, 3, 2) = (zFar + zNear) / (zNear  - zFar);
        glmMATFLOAT(&ortho, 3, 3) =  1.0f;
        ortho.type = glvFLOAT;

        /* Multipy the current matrix by the ortho matrix. */
        _MultiplyMatrix4x4(context->currentMatrix, &ortho, &result);

        /* Replace the current matrix. */
        *context->currentMatrix = result;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glOrthof(
    GLfloat Left,
    GLfloat Right,
    GLfloat Bottom,
    GLfloat Top,
    GLfloat zNear,
    GLfloat zFar
    )
{
	gcmDUMP_API("${ES11 glOrthof 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		*(GLuint*)&Left, *(GLuint*)&Right, *(GLuint*)&Bottom,
		*(GLuint*)&Top, *(GLuint*)&zNear, *(GLuint*)&zFar);

    _Orthof(Left, Right, Bottom, Top, zNear, zFar);
}

GL_API void GL_APIENTRY glOrthofOES(
    GLfloat Left,
    GLfloat Right,
    GLfloat Bottom,
    GLfloat Top,
    GLfloat zNear,
    GLfloat zFar
    )
{
	gcmDUMP_API("${ES11 glOrthofOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		*(GLuint*)&Left, *(GLuint*)&Right, *(GLuint*)&Bottom,
		*(GLuint*)&Top, *(GLuint*)&zNear, *(GLuint*)&zFar);

	_Orthof(Left, Right, Bottom, Top, zNear, zFar);
}
#endif


static void _Translatex(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
    glmENTER3(glmARGFIXED, X, glmARGFIXED, Y, glmARGFIXED, Z)
    {
	    glmPROFILE(context, GLES1_TRANSLATEX, 0);
        /* Optimization: do nothing when translation is zero. */
        if ((X == 0) && (Y == 0) && (Z == 0))
        {
            break;
        }

        if (context->currentMatrix->identity)
        {
            /*  Optimized result with identity:

                [3,0] = X
                [3,1] = Y
                [3,2] = Z
            */

#if !defined(COMMON_LITE)
            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 3, 0) = glmFIXED2FLOAT(X);
                glmMATFLOAT(context->currentMatrix, 3, 1) = glmFIXED2FLOAT(Y);
                glmMATFLOAT(context->currentMatrix, 3, 2) = glmFIXED2FLOAT(Z);
            }
            else
#endif
            {
                glmMATFIXED(context->currentMatrix, 3, 0) = X;
                glmMATFIXED(context->currentMatrix, 3, 1) = Y;
                glmMATFIXED(context->currentMatrix, 3, 2) = Z;
            }
        }
        else
        {
            /*  Optimized result:

            [3,0] = [0,0] * X + [1,0] * Y + [2,0] * Z + [3,0]
            [3,1] = [0,1] * X + [1,1] * Y + [2,1] * Z + [3,1]
            [3,2] = [0,2] * X + [1,2] * Y + [2,2] * Z + [3,2]
            [3,3] = [0,3] * X + [1,3] * Y + [2,3] * Z + [3,3]
            */

#if !defined(COMMON_LITE)
            if (context->currentMatrix->type == glvFLOAT)
            {
                GLfloat x = glmFIXED2FLOAT(X);
                GLfloat y = glmFIXED2FLOAT(Y);
                GLfloat z = glmFIXED2FLOAT(Z);

                glmMATFLOAT(context->currentMatrix, 3, 0) +=
                    glmMATFLOAT(context->currentMatrix, 0, 0) * x +
                    glmMATFLOAT(context->currentMatrix, 1, 0) * y +
                    glmMATFLOAT(context->currentMatrix, 2, 0) * z ;
                glmMATFLOAT(context->currentMatrix, 3, 1) +=
                    glmMATFLOAT(context->currentMatrix, 0, 1) * x +
                    glmMATFLOAT(context->currentMatrix, 1, 1) * y +
                    glmMATFLOAT(context->currentMatrix, 2, 1) * z ;
                glmMATFLOAT(context->currentMatrix, 3, 2) +=
                    glmMATFLOAT(context->currentMatrix, 0, 2) * x +
                    glmMATFLOAT(context->currentMatrix, 1, 2) * y +
                    glmMATFLOAT(context->currentMatrix, 2, 2) * z ;
                glmMATFLOAT(context->currentMatrix, 3, 3) +=
                    glmMATFLOAT(context->currentMatrix, 0, 3) * x +
                    glmMATFLOAT(context->currentMatrix, 1, 3) * y +
                    glmMATFLOAT(context->currentMatrix, 2, 3) * z ;
            }
            else
#endif
            {
                glmMATFIXED(context->currentMatrix, 3, 0) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 0),
                                 X) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 0),
                                 Y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 0),
                                 Z) ;
                glmMATFIXED(context->currentMatrix, 3, 1) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 1),
                                 X) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 1),
                                 Y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 1),
                                 Z) ;
                glmMATFIXED(context->currentMatrix, 3, 2) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 2),
                                 X) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 2),
                                 Y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 2),
                                 Z) ;
                glmMATFIXED(context->currentMatrix, 3, 3) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 3),
                                 X) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 3),
                                 Y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 3),
                                 Z) ;
            }
        }

        context->currentMatrix->identity = GL_FALSE;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glTranslate
**
**  glTranslate produces a translation by (x, y, z). The current matrix
**  (see glMatrixMode) is multiplied by this translation matrix, with
**  the product replacing the current matrix, as if glMultMatrix were called
**  with the following matrix for its argument:
**
**      1   0   0   x
**      0   1   0   y
**      0   0   1   z
**      0   0   0   1
**
**  If the matrix mode is either GL_MODELVIEW or GL_PROJECTION, all objects
**  drawn after a call to glTranslate are translated.
**
**  Use glPushMatrix and glPopMatrix to save and restore the untranslated
**  coordinate system.
**
**  INPUT:
**
**      X
**      Y
**      Z
**          Specify the x, y, and z coordinates of a translation vector.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glTranslatex(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
	gcmDUMP_API("${ES11 glTranslatex 0x%08X 0x%08X 0x%08X}", X, Y, Z);

    _Translatex(X, Y, Z);
}

GL_API void GL_APIENTRY glTranslatexOES(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
	gcmDUMP_API("${ES11 glTranslatexOES 0x%08X 0x%08X 0x%08X}", X, Y, Z);

	_Translatex(X, Y, Z);
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glTranslatef(
    GLfloat X,
    GLfloat Y,
    GLfloat Z
    )
{
    glmENTER3(glmARGFLOAT, X, glmARGFLOAT, Y, glmARGFLOAT, Z)
    {
		gcmDUMP_API("${ES11 glTranslatef 0x%08X 0x%08X 0x%08X}", *(GLuint*)&X, *(GLuint*)&Y, *(GLuint*)&Z);

		glmPROFILE(context, GLES1_TRANSLATEF, 0);
        /* Optimization: do nothing when translation is zero. */
        if ((X == 0.0f) && (Y == 0.0f) && (Z == 0.0f))
        {
            break;
        }

        if (context->currentMatrix->identity)
        {
            /*  Optimized result with identity:

                [3,0] = X
                [3,1] = Y
                [3,2] = Z
            */

            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 3, 0) = X;
                glmMATFLOAT(context->currentMatrix, 3, 1) = Y;
                glmMATFLOAT(context->currentMatrix, 3, 2) = Z;
            }
            else
            {
                glmMATFIXED(context->currentMatrix, 3, 0) = glmFLOAT2FIXED(X);
                glmMATFIXED(context->currentMatrix, 3, 1) = glmFLOAT2FIXED(Y);
                glmMATFIXED(context->currentMatrix, 3, 2) = glmFLOAT2FIXED(Z);
            }
        }
        else
        {
            /*  Optimized result:

            [3,0] = [0,0] * X + [1,0] * Y + [2,0] * Z + [3,0]
            [3,1] = [0,1] * X + [1,1] * Y + [2,1] * Z + [3,1]
            [3,2] = [0,2] * X + [1,2] * Y + [2,2] * Z + [3,2]
            [3,3] = [0,3] * X + [1,3] * Y + [2,3] * Z + [3,3]
            */

            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 3, 0) +=
                    glmMATFLOAT(context->currentMatrix, 0, 0) * X +
                    glmMATFLOAT(context->currentMatrix, 1, 0) * Y +
                    glmMATFLOAT(context->currentMatrix, 2, 0) * Z ;
                glmMATFLOAT(context->currentMatrix, 3, 1) +=
                    glmMATFLOAT(context->currentMatrix, 0, 1) * X +
                    glmMATFLOAT(context->currentMatrix, 1, 1) * Y +
                    glmMATFLOAT(context->currentMatrix, 2, 1) * Z ;
                glmMATFLOAT(context->currentMatrix, 3, 2) +=
                    glmMATFLOAT(context->currentMatrix, 0, 2) * X +
                    glmMATFLOAT(context->currentMatrix, 1, 2) * Y +
                    glmMATFLOAT(context->currentMatrix, 2, 2) * Z ;
                glmMATFLOAT(context->currentMatrix, 3, 3) +=
                    glmMATFLOAT(context->currentMatrix, 0, 3) * X +
                    glmMATFLOAT(context->currentMatrix, 1, 3) * Y +
                    glmMATFLOAT(context->currentMatrix, 2, 3) * Z ;
            }
            else
            {
                GLfixed x = glmFLOAT2FIXED(X);
                GLfixed y = glmFLOAT2FIXED(Y);
                GLfixed z = glmFLOAT2FIXED(Z);

                glmMATFIXED(context->currentMatrix, 3, 0) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 0), x) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 0), y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 0), z) ;
                glmMATFIXED(context->currentMatrix, 3, 1) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 1), x) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 1), y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 1), z) ;
                glmMATFIXED(context->currentMatrix, 3, 2) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 2), x) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 2), y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 2), z) ;
                glmMATFIXED(context->currentMatrix, 3, 3) +=
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 3), x) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 3), y) +
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 3), z) ;
            }
        }

        context->currentMatrix->identity = GL_FALSE;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}
#endif

static void _Rotatex(
    GLfixed Angle,
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
    glmENTER4(glmARGFIXED, Angle, glmARGFIXED, X, glmARGFIXED, Y,
              glmARGFIXED, Z)
    {
        GLfixed rad;
        GLfixed s, c, c1;
        glsVECTOR normal;
        GLfixed nx, ny, nz;
        GLfixed xx_1c, xy_1c, xz_1c, yy_1c, yz_1c, zz_1c;
        GLfixed xs, ys, zs;

	    glmPROFILE(context, GLES1_ROTATEX, 0);

        /* Optimization: do nothing when rotation is 0 or axis is origin. */
        if ((Angle == 0)
        ||  ((X == 0) && (Y == 0) && (Z == 0))
        )
        {
            break;
        }

        rad = gcmXMultiply(glvFIXEDPIOVER180, Angle);

        s  = glfSinX(rad);
        c  = glfCosX(rad);
        c1 = gcvONE_X - c;

        _Norm3X(X, Y, Z, &normal);

        nx = glmVECFIXED(&normal, 0);
        ny = glmVECFIXED(&normal, 1);
        nz = glmVECFIXED(&normal, 2);

        xx_1c = glmFIXEDMULTIPLY3(nx, nx, c1);
        xy_1c = glmFIXEDMULTIPLY3(nx, ny, c1);
        xz_1c = glmFIXEDMULTIPLY3(nx, nz, c1);
        yy_1c = glmFIXEDMULTIPLY3(ny, ny, c1);
        yz_1c = glmFIXEDMULTIPLY3(ny, nz, c1);
        zz_1c = glmFIXEDMULTIPLY3(nz, nz, c1);

        xs = gcmXMultiply(nx, s);
        ys = gcmXMultiply(ny, s);
        zs = gcmXMultiply(nz, s);

        if (context->currentMatrix->identity)
        {
            /*  Optimized result with identity:

                [0,0] = xx_1c + c
                [0,1] = xy_1c + zs
                [0,2] = xz_1c - ys
                [1,0] = xy_1c - zs
                [1,1] = yy_1c + c
                [1,2] = yz_1c + xs
                [2,0] = xz_1c + ys
                [2,1] = yz_1c - xs
                [2,2] = zz_1c + c
            */

#if !defined(COMMON_LITE)
            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 0, 0) =
                    glmFIXED2FLOAT(xx_1c + c );
                glmMATFLOAT(context->currentMatrix, 0, 1) =
                    glmFIXED2FLOAT(xy_1c + zs);
                glmMATFLOAT(context->currentMatrix, 0, 2) =
                    glmFIXED2FLOAT(xz_1c - ys);
                glmMATFLOAT(context->currentMatrix, 1, 0) =
                    glmFIXED2FLOAT(xy_1c - zs);
                glmMATFLOAT(context->currentMatrix, 1, 1) =
                    glmFIXED2FLOAT(yy_1c + c );
                glmMATFLOAT(context->currentMatrix, 1, 2) =
                    glmFIXED2FLOAT(yz_1c + xs);
                glmMATFLOAT(context->currentMatrix, 2, 0) =
                    glmFIXED2FLOAT(xz_1c + ys);
                glmMATFLOAT(context->currentMatrix, 2, 1) =
                    glmFIXED2FLOAT(yz_1c - xs);
                glmMATFLOAT(context->currentMatrix, 2, 2) =
                    glmFIXED2FLOAT(zz_1c + c );
            }
            else
#endif
            {
                glmMATFIXED(context->currentMatrix, 0, 0) = xx_1c + c;
                glmMATFIXED(context->currentMatrix, 0, 1) = xy_1c + zs;
                glmMATFIXED(context->currentMatrix, 0, 2) = xz_1c - ys;
                glmMATFIXED(context->currentMatrix, 1, 0) = xy_1c - zs;
                glmMATFIXED(context->currentMatrix, 1, 1) = yy_1c + c;
                glmMATFIXED(context->currentMatrix, 1, 2) = yz_1c + xs;
                glmMATFIXED(context->currentMatrix, 2, 0) = xz_1c + ys;
                glmMATFIXED(context->currentMatrix, 2, 1) = yz_1c - xs;
                glmMATFIXED(context->currentMatrix, 2, 2) = zz_1c + c;
            }
        }
        else
        {
            /*  Optimized result:

                [0,0] = [0,0] * (xx_1c + c)  + [1,0] * (xy_1c + zs) + [2,0] * (xz_1c - ys)
                [0,1] = [0,1] * (xx_1c + c)  + [1,1] * (xy_1c + zs) + [2,1] * (xz_1c - ys)
                [0,2] = [0,2] * (xx_1c + c)  + [1,2] * (xy_1c + zs) + [2,2] * (xz_1c - ys)
                [0,3] = [0,3] * (xx_1c + c)  + [1,3] * (xy_1c + zs) + [2,3] * (xz_1c - ys)
                [1,0] = [0,0] * (xy_1c - zs) + [1,0] * (yy_1c + c)  + [2,0] * (yz_1c + xs)
                [1,1] = [0,1] * (xy_1c - zs) + [1,1] * (yy_1c + c)  + [2,1] * (yz_1c + xs)
                [1,2] = [0,2] * (xy_1c - zs) + [1,2] * (yy_1c + c)  + [2,2] * (yz_1c + xs)
                [1,3] = [0,3] * (xy_1c - zs) + [1,3] * (yy_1c + c)  + [2,3] * (yz_1c + xs)
                [2,0] = [0,0] * (xz_1c + ys) + [1,0] * (yz_1c - xs) + [2,0] * (zz_1c + c)
                [2,1] = [0,1] * (xz_1c + ys) + [1,1] * (yz_1c - xs) + [2,1] * (zz_1c + c)
                [2,2] = [0,2] * (xz_1c + ys) + [1,2] * (yz_1c - xs) + [2,2] * (zz_1c + c)
                [2,3] = [0,3] * (xz_1c + ys) + [1,3] * (yz_1c - xs) + [2,3] * (zz_1c + c)
            */

            GLint x, y;

#if !defined(COMMON_LITE)
            if (context->currentMatrix->type == glvFLOAT)
            {
                GLfloat rotation[3][3];
                glsMATRIX source = *context->currentMatrix;

                rotation[0][0] = glmFIXED2FLOAT(xx_1c + c);
                rotation[0][1] = glmFIXED2FLOAT(xy_1c + zs);
                rotation[0][2] = glmFIXED2FLOAT(xz_1c - ys);
                rotation[1][0] = glmFIXED2FLOAT(xy_1c - zs);
                rotation[1][1] = glmFIXED2FLOAT(yy_1c + c);
                rotation[1][2] = glmFIXED2FLOAT(yz_1c + xs);
                rotation[2][0] = glmFIXED2FLOAT(xz_1c + ys);
                rotation[2][1] = glmFIXED2FLOAT(yz_1c - xs);
                rotation[2][2] = glmFIXED2FLOAT(zz_1c + c);

                for (x = 0; x < 3; ++x)
                {
                    for (y = 0; y < 4; ++y)
                    {
                        glmMATFLOAT(context->currentMatrix, x, y) =
                            glmMATFLOAT(&source, 0, y) * rotation[x][0] +
                            glmMATFLOAT(&source, 1, y) * rotation[x][1] +
                            glmMATFLOAT(&source, 2, y) * rotation[x][2] ;
                    }
                }
            }
            else
#endif
            {
                GLfixed rotation[3][3];
                glsMATRIX source = *context->currentMatrix;

                rotation[0][0] = xx_1c + c;
                rotation[0][1] = xy_1c + zs;
                rotation[0][2] = xz_1c - ys;
                rotation[1][0] = xy_1c - zs;
                rotation[1][1] = yy_1c + c;
                rotation[1][2] = yz_1c + xs;
                rotation[2][0] = xz_1c + ys;
                rotation[2][1] = yz_1c - xs;
                rotation[2][2] = zz_1c + c;

                for (x = 0; x < 3; ++x)
                {
                    for (y = 0; y < 4; ++y)
                    {
                        glmMATFIXED(context->currentMatrix, x, y) =
                            gcmXMultiply(glmMATFIXED(&source, 0, y),
                                         rotation[x][0]) +
                            gcmXMultiply(glmMATFIXED(&source, 1, y),
                                         rotation[x][1]) +
                            gcmXMultiply(glmMATFIXED(&source, 2, y),
                                         rotation[x][2]) ;
                    }
                }
            }
        }

        context->currentMatrix->identity = GL_FALSE;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}



/*******************************************************************************
**
**  glRotate
**
**  glRotate produces a rotation of Angle degrees around the vector (x, y, z).
**  The current matrix (see glMatrixMode) is multiplied by a rotation matrix
**  with the product replacing the current matrix, as if glMultMatrix were
**  called with the following matrix as its argument:
**
**      xx (1 - c) + c      xy (1 - c) - zs     xz (1 - c) + ys     0
**      xy (1 - c) + zs     yy (1 - c) + c      yz (1 - c) - xs     0
**      xz (1 - c) - ys     yz (1 - c) + xs     zz (1 - c) + c      0
**      0                   0                   0                   1
**
**  Where c = cos(Angle), s = sin(Angle), and || (x, y, z) || = 1, (if not,
**  the GL will normalize this vector).
**
**  If the matrix mode is either GL_MODELVIEW or GL_PROJECTION, all objects
**  drawn after glRotate is called are rotated. Use glPushMatrix and
**  glPopMatrix to save and restore the unrotated coordinate system.
**
**  INPUT:
**
**      Angle
**          Specifies the angle of rotation, in degrees.
**
**      X
**      Y
**      Z
**          Specify the x, y, and z coordinates of a vector, respectively.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glRotatex(
    GLfixed Angle,
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
	gcmDUMP_API("${ES11 glRotatex 0x%08X 0x%08X 0x%08X 0x%08X}", Angle, X, Y, Z);

	_Rotatex(Angle, X, Y, Z);
}

GL_API void GL_APIENTRY glRotatexOES(
    GLfixed Angle,
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
	gcmDUMP_API("${ES11 glRotatexOES 0x%08X 0x%08X 0x%08X 0x%08X}", Angle, X, Y, Z);

	_Rotatex(Angle, X, Y, Z);
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glRotatef(
    GLfloat Angle,
    GLfloat X,
    GLfloat Y,
    GLfloat Z
    )
{
    glmENTER4(glmARGFLOAT, Angle, glmARGFLOAT, X, glmARGFLOAT, Y,
              glmARGFLOAT, Z)
    {
        GLfloat rad;
        GLfloat s, c, c1;
        glsVECTOR normal;
        GLfloat nx, ny, nz;
        GLfloat xx_1c, xy_1c, xz_1c, yy_1c, yz_1c, zz_1c;
        GLfloat xs, ys, zs;

		gcmDUMP_API("${ES11 glRotatef 0x%08X 0x%08X 0x%08X 0x%08X}",
			*(GLuint*)&Angle, *(GLuint*)&X, *(GLuint*)&Y, *(GLuint*)&Z);

        glmPROFILE(context, GLES1_ROTATEF, 0);
        /* Optimization: do nothing when rotation is 0 or axis is origin. */
        if ((Angle == 0.0f)
        ||  ((X == 0.0f) && (Y == 0.0f) && (Z == 0.0f))
        )
        {
            break;
        }

        rad = glvFLOATPIOVER180 * Angle;

        s  = gcoMATH_Sine(rad);
        c  = gcoMATH_Cosine(rad);
        c1 = 1.0f - c;

        _Norm3F(X, Y, Z, &normal);

        nx = glmVECFLOAT(&normal, 0);
        ny = glmVECFLOAT(&normal, 1);
        nz = glmVECFLOAT(&normal, 2);

        xx_1c = nx * nx * c1;
        xy_1c = nx * ny * c1;
        xz_1c = nx * nz * c1;
        yy_1c = ny * ny * c1;
        yz_1c = ny * nz * c1;
        zz_1c = nz * nz * c1;

        xs = nx * s;
        ys = ny * s;
        zs = nz * s;

        if (context->currentMatrix->identity)
        {
            /*  Optimized result with identity:

                [0,0] = xx_1c + c
                [0,1] = xy_1c + zs
                [0,2] = xz_1c - ys
                [1,0] = xy_1c - zs
                [1,1] = yy_1c + c
                [1,2] = yz_1c + xs
                [2,0] = xz_1c + ys
                [2,1] = yz_1c - xs
                [2,2] = zz_1c + c
            */

            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 0, 0) = xx_1c + c;
                glmMATFLOAT(context->currentMatrix, 0, 1) = xy_1c + zs;
                glmMATFLOAT(context->currentMatrix, 0, 2) = xz_1c - ys;
                glmMATFLOAT(context->currentMatrix, 1, 0) = xy_1c - zs;
                glmMATFLOAT(context->currentMatrix, 1, 1) = yy_1c + c;
                glmMATFLOAT(context->currentMatrix, 1, 2) = yz_1c + xs;
                glmMATFLOAT(context->currentMatrix, 2, 0) = xz_1c + ys;
                glmMATFLOAT(context->currentMatrix, 2, 1) = yz_1c - xs;
                glmMATFLOAT(context->currentMatrix, 2, 2) = zz_1c + c;
            }
            else
            {
                glmMATFIXED(context->currentMatrix, 0, 0) =
                    glmFLOAT2FIXED(xx_1c + c);
                glmMATFIXED(context->currentMatrix, 0, 1) =
                    glmFLOAT2FIXED(xy_1c + zs);
                glmMATFIXED(context->currentMatrix, 0, 2) =
                    glmFLOAT2FIXED(xz_1c - ys);
                glmMATFIXED(context->currentMatrix, 1, 0) =
                    glmFLOAT2FIXED(xy_1c - zs);
                glmMATFIXED(context->currentMatrix, 1, 1) =
                    glmFLOAT2FIXED(yy_1c + c);
                glmMATFIXED(context->currentMatrix, 1, 2) =
                    glmFLOAT2FIXED(yz_1c + xs);
                glmMATFIXED(context->currentMatrix, 2, 0) =
                    glmFLOAT2FIXED(xz_1c + ys);
                glmMATFIXED(context->currentMatrix, 2, 1) =
                    glmFLOAT2FIXED(yz_1c - xs);
                glmMATFIXED(context->currentMatrix, 2, 2) =
                    glmFLOAT2FIXED(zz_1c + c);
            }
        }
        else
        {
            /*  Optimized result:

                [0,0] = [0,0] * (xx_1c + c)  + [1,0] * (xy_1c + zs) + [2,0] * (xz_1c - ys)
                [0,1] = [0,1] * (xx_1c + c)  + [1,1] * (xy_1c + zs) + [2,1] * (xz_1c - ys)
                [0,2] = [0,2] * (xx_1c + c)  + [1,2] * (xy_1c + zs) + [2,2] * (xz_1c - ys)
                [0,3] = [0,3] * (xx_1c + c)  + [1,3] * (xy_1c + zs) + [2,3] * (xz_1c - ys)
                [1,0] = [0,0] * (xy_1c - zs) + [1,0] * (yy_1c + c)  + [2,0] * (yz_1c + xs)
                [1,1] = [0,1] * (xy_1c - zs) + [1,1] * (yy_1c + c)  + [2,1] * (yz_1c + xs)
                [1,2] = [0,2] * (xy_1c - zs) + [1,2] * (yy_1c + c)  + [2,2] * (yz_1c + xs)
                [1,3] = [0,3] * (xy_1c - zs) + [1,3] * (yy_1c + c)  + [2,3] * (yz_1c + xs)
                [2,0] = [0,0] * (xz_1c + ys) + [1,0] * (yz_1c - xs) + [2,0] * (zz_1c + c)
                [2,1] = [0,1] * (xz_1c + ys) + [1,1] * (yz_1c - xs) + [2,1] * (zz_1c + c)
                [2,2] = [0,2] * (xz_1c + ys) + [1,2] * (yz_1c - xs) + [2,2] * (zz_1c + c)
                [2,3] = [0,3] * (xz_1c + ys) + [1,3] * (yz_1c - xs) + [2,3] * (zz_1c + c)
            */

            GLint x, y;

            if (context->currentMatrix->type == glvFLOAT)
            {
                GLfloat rotation[3][3];
                glsMATRIX source = *context->currentMatrix;

                rotation[0][0] = xx_1c + c;
                rotation[0][1] = xy_1c + zs;
                rotation[0][2] = xz_1c - ys;
                rotation[1][0] = xy_1c - zs;
                rotation[1][1] = yy_1c + c;
                rotation[1][2] = yz_1c + xs;
                rotation[2][0] = xz_1c + ys;
                rotation[2][1] = yz_1c - xs;
                rotation[2][2] = zz_1c + c;

                for (x = 0; x < 3; ++x)
                {
                    for (y = 0; y < 4; ++y)
                    {
                        glmMATFLOAT(context->currentMatrix, x, y) =
                            glmMATFLOAT(&source, 0, y) * rotation[x][0] +
                            glmMATFLOAT(&source, 1, y) * rotation[x][1] +
                            glmMATFLOAT(&source, 2, y) * rotation[x][2] ;
                    }
                }
            }
            else
            {
                GLfixed rotation[3][3];
                glsMATRIX source = *context->currentMatrix;

                rotation[0][0] = glmFLOAT2FIXED(xx_1c + c);
                rotation[0][1] = glmFLOAT2FIXED(xy_1c + zs);
                rotation[0][2] = glmFLOAT2FIXED(xz_1c - ys);
                rotation[1][0] = glmFLOAT2FIXED(xy_1c - zs);
                rotation[1][1] = glmFLOAT2FIXED(yy_1c + c);
                rotation[1][2] = glmFLOAT2FIXED(yz_1c + xs);
                rotation[2][0] = glmFLOAT2FIXED(xz_1c + ys);
                rotation[2][1] = glmFLOAT2FIXED(yz_1c - xs);
                rotation[2][2] = glmFLOAT2FIXED(zz_1c + c);

                for (x = 0; x < 3; ++x)
                {
                    for (y = 0; y < 4; ++y)
                    {
                        glmMATFIXED(context->currentMatrix, x, y) =
                            gcmXMultiply(glmMATFIXED(&source, 0, y),
                                         rotation[x][0]) +
                            gcmXMultiply(glmMATFIXED(&source, 1, y),
                                         rotation[x][1]) +
                            gcmXMultiply(glmMATFIXED(&source, 2, y),
                                         rotation[x][2]) ;
                    }
                }
            }
        }

        context->currentMatrix->identity = GL_FALSE;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}
#endif


static void _Frustumx(
    GLfixed Left,
    GLfixed Right,
    GLfixed Bottom,
    GLfixed Top,
    GLfixed zNear,
    GLfixed zFar
    )
{
    glmENTER6(glmARGFIXED, Left, glmARGFIXED, Right, glmARGFIXED, Bottom,
              glmARGFIXED, Top, glmARGFIXED, zNear, glmARGFIXED, zFar)
    {
        glsMATRIX frustum, result;
#ifndef COMMON_LITE
        GLfloat _Left, _Right, _Bottom, _Top, _zNear, _zFar;
#endif

	    glmPROFILE(context, GLES1_FRUSTUMX, 0);

        /* Verify arguments. */
        if ((Left == Right) ||
            (Bottom == Top) ||
            (zNear <= 0)    ||
            (zFar <= 0)     ||
            (zNear == zFar))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Fill in frustum matrix. */
        gcoOS_ZeroMemory(&frustum, sizeof(frustum));
#ifndef COMMON_LITE
        _Left   = glmFIXED2FLOAT(Left);
        _Right  = glmFIXED2FLOAT(Right);
        _Bottom = glmFIXED2FLOAT(Bottom);
        _Top    = glmFIXED2FLOAT(Top);
        _zNear  = glmFIXED2FLOAT(zNear);
        _zFar   = glmFIXED2FLOAT(zFar);

        glmMATFLOAT(&frustum, 0, 0) =  2.0f * _zNear         / (_Right - _Left);
        glmMATFLOAT(&frustum, 1, 1) =  2.0f * _zNear         / (_Top   - _Bottom);
        glmMATFLOAT(&frustum, 2, 0) = (_Right + _Left)       / (_Right - _Left);
        glmMATFLOAT(&frustum, 2, 1) = (_Top   + _Bottom)     / (_Top   - _Bottom);
        glmMATFLOAT(&frustum, 2, 2) = (_zNear + _zFar)       / (_zNear - _zFar);
        glmMATFLOAT(&frustum, 2, 3) = -1.0f;
        glmMATFLOAT(&frustum, 3, 2) =  2.0f * _zNear * _zFar / (_zNear - _zFar);
        frustum.type = glvFLOAT;
#else
        glmMATFIXED(&frustum, 0, 0) = glmFIXEDDIVIDE(glmFIXEDMULTIPLY(gcvTWO_X, zNear), Right - Left);
        glmMATFIXED(&frustum, 1, 1) = glmFIXEDDIVIDE(glmFIXEDMULTIPLY(gcvTWO_X, zNear), Top   - Bottom);
        glmMATFIXED(&frustum, 2, 0) = glmFIXEDDIVIDE(Right + Left,   Right - Left);
        glmMATFIXED(&frustum, 2, 1) = glmFIXEDDIVIDE(Top   + Bottom, Top   - Bottom);
        glmMATFIXED(&frustum, 2, 2) = glmFIXEDDIVIDE(zNear + zFar,   zNear - zFar);
        glmMATFIXED(&frustum, 2, 3) = gcvNEGONE_X;
        glmMATFIXED(&frustum, 3, 2) = glmFIXEDMULTIPLYDIVIDE(zNear + zNear, zFar, zNear - zFar);
        frustum.type = glvFIXED;
#endif

        /* Multipy the current matrix by the frustum matrix. */
        if(_MultiplyMatrix4x4(context->currentMatrix, &frustum, &result))
        {
            /* Replace the current matrix. */
            *context->currentMatrix = result;

            /* Notify dependents of the change. */
            (*context->currentStack->dataChanged) (context);
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glFrustum
**
**  glFrustum describes a perspective matrix that produces a perspective
**  projection. The current matrix (see glMatrixMode) is multiplied by this
**  matrix and the result replaces the current matrix, as if glMultMatrix were
**  called with the following matrix as its argument:
**
**      2 * near / (right - left)    0                            A    0
**      0                            2 * near / (top - bottom)    B    0
**      0                            0                            C    D
**      0                            0                           -1    0
**
**  where
**
**      A = (right + left)   / (right - left)
**      B = (top   + bottom) / (top   - bottom)
**      C = (near  + far)    / (near  - far)
**      D = 2 * near * far   / (near  - far)
**
**  Typically, the matrix mode is GL_PROJECTION, and (left, bottom, -near) and
**  (right, top, -near) specify the points on the near clipping plane that are
**  mapped to the lower left and upper right corners of the window, assuming
**  that the eye is located at (0, 0, 0). -far specifies the location of the
**  far clipping plane. Both near and far must be positive.
**
**  Use glPushMatrix and glPopMatrix to save and restore the current
**  matrix stack.
**
**  INPUT:
**
**      Left
**      Right
**          Specify the coordinates for the left and right vertical
**          clipping planes.
**
**      Bottom
**      Right
**          Specify the coordinates for the bottom and top horizontal
**          clipping planes.
**
**      zNear
**      zFar
**          Specify the distances to the near and far depth clipping planes.
**          Both distances must be positive.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glFrustumx(
    GLfixed Left,
    GLfixed Right,
    GLfixed Bottom,
    GLfixed Top,
    GLfixed zNear,
    GLfixed zFar
    )
{
	gcmDUMP_API("${ES11 glFrustumx 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		Left, Right, Bottom, Top, zNear, zFar);

    _Frustumx(Left, Right, Bottom, Top, zNear, zFar);
}

GL_API void GL_APIENTRY glFrustumxOES(
    GLfixed Left,
    GLfixed Right,
    GLfixed Bottom,
    GLfixed Top,
    GLfixed zNear,
    GLfixed zFar
    )
{
	gcmDUMP_API("${ES11 glFrustumxOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		Left, Right, Bottom, Top, zNear, zFar);

	_Frustumx(Left, Right, Bottom, Top, zNear, zFar);
}

#ifndef COMMON_LITE
static void _Frustumf(
    GLfloat Left,
    GLfloat Right,
    GLfloat Bottom,
    GLfloat Top,
    GLfloat zNear,
    GLfloat zFar
    )
{
    glmENTER6(glmARGFLOAT, Left, glmARGFLOAT, Right, glmARGFLOAT, Bottom,
              glmARGFLOAT, Top, glmARGFLOAT, zNear, glmARGFLOAT, zFar)
    {
        glsMATRIX frustum, result;

	    glmPROFILE(context, GLES1_FRUSTUMF, 0);

        /* Verify arguments. */
        if ((Left == Right) ||
            (Bottom == Top) ||
            (zNear <= 0.0f) ||
            (zFar <= 0.0f)  ||
            (zNear == zFar))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Fill in frustum matrix. */
        gcoOS_ZeroMemory(&frustum, sizeof(frustum));
        glmMATFLOAT(&frustum, 0, 0) =  2.0f * zNear        / (Right - Left);
        glmMATFLOAT(&frustum, 1, 1) =  2.0f * zNear        / (Top   - Bottom);
        glmMATFLOAT(&frustum, 2, 0) = (Right + Left)       / (Right - Left);
        glmMATFLOAT(&frustum, 2, 1) = (Top   + Bottom)     / (Top   - Bottom);
        glmMATFLOAT(&frustum, 2, 2) = (zNear + zFar)       / (zNear - zFar);
        glmMATFLOAT(&frustum, 2, 3) = -1.0f;
        glmMATFLOAT(&frustum, 3, 2) =  2.0f * zNear * zFar / (zNear - zFar);
        frustum.type = glvFLOAT;

        /* Multipy the current matrix by the frustum matrix. */
        _MultiplyMatrix4x4(context->currentMatrix, &frustum, &result);

        /* Replace the current matrix. */
        *context->currentMatrix = result;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glFrustumf(
    GLfloat Left,
    GLfloat Right,
    GLfloat Bottom,
    GLfloat Top,
    GLfloat zNear,
    GLfloat zFar
    )
{
	gcmDUMP_API("${ES11 glFrustumf 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		*(GLuint*)&Left, *(GLuint*)&Right, *(GLuint*)&Bottom,
		*(GLuint*)&Top, *(GLuint*)&zNear, *(GLuint*)&zFar);

	_Frustumf(Left, Right, Bottom, Top, zNear, zFar);
}

GL_API void GL_APIENTRY glFrustumfOES(
    GLfloat Left,
    GLfloat Right,
    GLfloat Bottom,
    GLfloat Top,
    GLfloat zNear,
    GLfloat zFar
    )
{
	gcmDUMP_API("${ES11 glFrustumfOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
		*(GLuint*)&Left, *(GLuint*)&Right, *(GLuint*)&Bottom,
		*(GLuint*)&Top, *(GLuint*)&zNear, *(GLuint*)&zFar);

    _Frustumf(Left, Right, Bottom, Top, zNear, zFar);
}
#endif

static void _Scalex(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
    glmENTER3(glmARGFIXED, X, glmARGFIXED, Y, glmARGFIXED, Z)
    {
	    glmPROFILE(context, GLES1_SCALEX, 0);

        /* Optimization: do nothing when scale is one. */
        if ((X == gcvONE_X) && (Y == gcvONE_X) && (Z == gcvONE_X))
        {
            break;
        }

        if (context->currentMatrix->identity)
        {
            /*  Optimized result with identity:

                [0,0] = X
                [1,1] = Y
                [2,2] = Z
            */

#if !defined(COMMON_LITE)
            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 0, 0) = glmFIXED2FLOAT(X);
                glmMATFLOAT(context->currentMatrix, 1, 1) = glmFIXED2FLOAT(Y);
                glmMATFLOAT(context->currentMatrix, 2, 2) = glmFIXED2FLOAT(Z);
            }
            else
#endif
            {
                glmMATFIXED(context->currentMatrix, 0, 0) = X;
                glmMATFIXED(context->currentMatrix, 1, 1) = Y;
                glmMATFIXED(context->currentMatrix, 2, 2) = Z;
            }
        }
        else
        {
            /*  Optimized result:

            [0,0] = [0,0] * X
            [0,1] = [0,1] * X
            [0,2] = [0,2] * X
            [0,3] = [0,3] * X
            [1,0] = [1,0] * Y
            [1,1] = [1,1] * Y
            [1,2] = [1,2] * Y
            [1,3] = [1,3] * Y
            [2,0] = [2,0] * Z
            [2,1] = [2,1] * Z
            [2,2] = [2,2] * Z
            [2,3] = [2,3] * Z
            */

#if !defined(COMMON_LITE)
            if (context->currentMatrix->type == glvFLOAT)
            {
                GLfloat x = glmFIXED2FLOAT(X);
                GLfloat y = glmFIXED2FLOAT(Y);
                GLfloat z = glmFIXED2FLOAT(Z);

                glmMATFLOAT(context->currentMatrix, 0, 0) *= x;
                glmMATFLOAT(context->currentMatrix, 0, 1) *= x;
                glmMATFLOAT(context->currentMatrix, 0, 2) *= x;
                glmMATFLOAT(context->currentMatrix, 0, 3) *= x;
                glmMATFLOAT(context->currentMatrix, 1, 0) *= y;
                glmMATFLOAT(context->currentMatrix, 1, 1) *= y;
                glmMATFLOAT(context->currentMatrix, 1, 2) *= y;
                glmMATFLOAT(context->currentMatrix, 1, 3) *= y;
                glmMATFLOAT(context->currentMatrix, 2, 0) *= z;
                glmMATFLOAT(context->currentMatrix, 2, 1) *= z;
                glmMATFLOAT(context->currentMatrix, 2, 2) *= z;
                glmMATFLOAT(context->currentMatrix, 2, 3) *= z;
            }
            else
#endif
            {
                glmMATFIXED(context->currentMatrix, 0, 0) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 0), X);
                glmMATFIXED(context->currentMatrix, 0, 1) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 1), X);
                glmMATFIXED(context->currentMatrix, 0, 2) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 2), X);
                glmMATFIXED(context->currentMatrix, 0, 3) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 3), X);
                glmMATFIXED(context->currentMatrix, 1, 0) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 0), Y);
                glmMATFIXED(context->currentMatrix, 1, 1) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 1), Y);
                glmMATFIXED(context->currentMatrix, 1, 2) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 2), Y);
                glmMATFIXED(context->currentMatrix, 1, 3) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 3), Y);
                glmMATFIXED(context->currentMatrix, 2, 0) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 0), Z);
                glmMATFIXED(context->currentMatrix, 2, 1) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 1), Z);
                glmMATFIXED(context->currentMatrix, 2, 2) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 2), Z);
                glmMATFIXED(context->currentMatrix, 2, 3) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 3), Z);
            }
        }

        context->currentMatrix->identity = GL_FALSE;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}



/*******************************************************************************
**
**  glScale
**
**  glScale produces a nonuniform scaling along the x, y, and z axes. The three
**  parameters indicate the desired scale factor along each of the three axes.
**
**  The current matrix (see glMatrixMode) is multiplied by this scale matrix,
**  and the product replaces the current matrix as if glScale were called with
**  the following matrix as its argument:
**
**      x   0   0   0
**      0   y   0   0
**      0   0   z   0
**      0   0   0   1
**
**  If the matrix mode is either GL_MODELVIEW or GL_PROJECTION, all objects
**  drawn after glScale is called are scaled.
**
**  Use glPushMatrix and glPopMatrix to save and restore the unscaled
**  coordinate system.
**
**  INPUT:
**
**      X
**      Y
**      Z
**          Specify scale factors along the x, y, and z axes, respectively.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glScalex(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
	gcmDUMP_API("${ES11 glScalex 0x%08X 0x%08X 0x%08X}", X, Y, Z);

    _Scalex(X, Y, Z);
}

GL_API void GL_APIENTRY glScalexOES(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
	gcmDUMP_API("${ES11 glScalexOES 0x%08X 0x%08X 0x%08X}", X, Y, Z);

    _Scalex(X, Y, Z);
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glScalef(
    GLfloat X,
    GLfloat Y,
    GLfloat Z
    )
{
    glmENTER3(glmARGFLOAT, X, glmARGFLOAT, Y, glmARGFLOAT, Z)
    {
		gcmDUMP_API("${ES11 glScalef 0x%08X 0x%08X 0x%08X}",
			*(GLuint*)&X, *(GLuint*)&Y, *(GLuint*)&Z);

        glmPROFILE(context, GLES1_SCALEF, 0);
        /* Optimization: do nothing when scale is one. */
        if ((X == 1.0f) && (Y == 1.0f) && (Z == 1.0f))
        {
            break;
        }

        if (context->currentMatrix->identity)
        {
            /*  Optimized result with identity:

                [0,0] = X
                [1,1] = Y
                [2,2] = Z
            */

            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 0, 0) = X;
                glmMATFLOAT(context->currentMatrix, 1, 1) = Y;
                glmMATFLOAT(context->currentMatrix, 2, 2) = Z;
            }
            else
            {
                glmMATFIXED(context->currentMatrix, 0, 0) = glmFLOAT2FIXED(X);
                glmMATFIXED(context->currentMatrix, 1, 1) = glmFLOAT2FIXED(Y);
                glmMATFIXED(context->currentMatrix, 2, 2) = glmFLOAT2FIXED(Z);
            }
        }
        else
        {
            /*  Optimized result:

            [0,0] = [0,0] * X
            [1,0] = [1,0] * Y
            [2,0] = [2,0] * Z
            [0,1] = [0,1] * X
            [1,1] = [1,1] * Y
            [2,1] = [2,1] * Z
            [0,2] = [0,2] * X
            [1,2] = [1,2] * Y
            [2,2] = [2,2] * Z
            [0,3] = [0,3] * X
            [1,3] = [1,3] * Y
            [2,3] = [2,3] * Z
            */

            if (context->currentMatrix->type == glvFLOAT)
            {
                glmMATFLOAT(context->currentMatrix, 0, 0) *= X;
                glmMATFLOAT(context->currentMatrix, 0, 1) *= X;
                glmMATFLOAT(context->currentMatrix, 0, 2) *= X;
                glmMATFLOAT(context->currentMatrix, 0, 3) *= X;
                glmMATFLOAT(context->currentMatrix, 1, 0) *= Y;
                glmMATFLOAT(context->currentMatrix, 1, 1) *= Y;
                glmMATFLOAT(context->currentMatrix, 1, 2) *= Y;
                glmMATFLOAT(context->currentMatrix, 1, 3) *= Y;
                glmMATFLOAT(context->currentMatrix, 2, 0) *= Z;
                glmMATFLOAT(context->currentMatrix, 2, 1) *= Z;
                glmMATFLOAT(context->currentMatrix, 2, 2) *= Z;
                glmMATFLOAT(context->currentMatrix, 2, 3) *= Z;
            }
            else
            {
                GLfixed x = glmFLOAT2FIXED(X);
                GLfixed y = glmFLOAT2FIXED(Y);
                GLfixed z = glmFLOAT2FIXED(Z);

                glmMATFIXED(context->currentMatrix, 0, 0) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 0), x);
                glmMATFIXED(context->currentMatrix, 0, 1) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 1), x);
                glmMATFIXED(context->currentMatrix, 0, 2) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 2), x);
                glmMATFIXED(context->currentMatrix, 0, 3) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 0, 3), x);
                glmMATFIXED(context->currentMatrix, 1, 0) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 0), y);
                glmMATFIXED(context->currentMatrix, 1, 1) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 1), y);
                glmMATFIXED(context->currentMatrix, 1, 2) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 2), y);
                glmMATFIXED(context->currentMatrix, 1, 3) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 1, 3), y);
                glmMATFIXED(context->currentMatrix, 2, 0) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 0), z);
                glmMATFIXED(context->currentMatrix, 2, 1) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 1), z);
                glmMATFIXED(context->currentMatrix, 2, 2) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 2), z);
                glmMATFIXED(context->currentMatrix, 2, 3) =
                    gcmXMultiply(glmMATFIXED(context->currentMatrix, 2, 3), z);
            }
        }

        context->currentMatrix->identity = GL_FALSE;

        /* Notify dependents of the change. */
        (*context->currentStack->dataChanged) (context);
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glMultMatrix
**
**  glMultMatrix multiplies the current matrix with the one specified using
**  Matrix, and replaces the current matrix with the product.
**
**  The current matrix is determined by the current matrix mode
**  (see glMatrixMode). It is either the projection matrix, modelview matrix,
**  or the texture matrix.
**
**  INPUT:
**
**      Matrix
**          Points to 16 consecutive values that are used as the elements of
**          a 4 ? 4 column-major matrix.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glMultMatrixx(
    const GLfixed* Matrix
    )
{
    glmENTER1(glmARGPTR, Matrix)
    {
        glsMATRIX matrix, result;

		gcmDUMP_API("${ES11 glMultMatrixx (0x%08X)", Matrix);
		gcmDUMP_API_ARRAY(Matrix, 16);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_MULTMATRIXX, 0);

        /* Load the matrix. */
        if(_LoadMatrix(&matrix, glvFIXED, Matrix))
        {
            /* Multipy the current matrix by the matrix. */
            if(_MultiplyMatrix4x4(context->currentMatrix, &matrix, &result))
            {
                /* Replace the current matrix. */
                *context->currentMatrix = result;

                /* Notify dependents of the change. */
                (*context->currentStack->dataChanged) (context);
            }
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glMultMatrixxOES(
    const GLfixed* Matrix
    )
{
    glmENTER1(glmARGPTR, Matrix)
    {
        glsMATRIX matrix, result;

		gcmDUMP_API("${ES11 glMultMatrixxOES (0x%08X)", Matrix);
		gcmDUMP_API_ARRAY(Matrix, 16);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_MULTMATRIXX, 0);

        /* Load the matrix. */
        if(_LoadMatrix(&matrix, glvFIXED, Matrix))
        {
            /* Multipy the current matrix by the matrix. */
            if(_MultiplyMatrix4x4(context->currentMatrix, &matrix, &result))
            {
                /* Replace the current matrix. */
                *context->currentMatrix = result;

                /* Notify dependents of the change. */
                (*context->currentStack->dataChanged) (context);
            }
        }
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glMultMatrixf(
    const GLfloat* Matrix
    )
{
    glmENTER1(glmARGPTR, Matrix)
    {
        glsMATRIX matrix, result;

		gcmDUMP_API("${ES11 glMultMatrixf (0x%08X)", Matrix);
		gcmDUMP_API_ARRAY(Matrix, 16);
		gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_MULTMATRIXF, 0);

        /* Load the matrix. */
        if(_LoadMatrix(&matrix, glvFLOAT, Matrix))
        {
            /* Multipy the current matrix by the matrix. */
            if(_MultiplyMatrix4x4(context->currentMatrix, &matrix, &result))
            {
                /* Replace the current matrix. */
                *context->currentMatrix = result;

                /* Notify dependents of the change. */
                (*context->currentStack->dataChanged) (context);
            }
        }
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glPushMatrix/glPopMatrix
**
**  There is a stack of matrices for each of the matrix modes. In GL_MODELVIEW
**  mode, the stack depth is at least 16. In the other modes, GL_PROJECTION,
**  and GL_TEXTURE, the depth is at least 2. The current matrix in any mode
**  is the matrix on the top of the stack for that mode.
**
**  glPushMatrix pushes the current matrix stack down by one, duplicating
**  the current matrix. That is, after a glPushMatrix call, the matrix on top
**  of the stack is identical to the one below it.
**
**  glPopMatrix pops the current matrix stack, replacing the current matrix
**  with the one below it on the stack.
**
**  Initially, each of the stacks contains one matrix, an identity matrix.
**
**  It is an error to push a full matrix stack, or to pop a matrix stack that
**  contains only a single matrix. In either case, the error flag is set and
**  no other change is made to GL state.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glPushMatrix(
    void
    )
{
    glmENTER()
    {
		gcmDUMP_API("${ES11 glPushMatrix}");

		glmPROFILE(context, GLES1_PUSHMATRIX, 0);
        /* Stack overflow? */
        if (context->currentStack->index == context->currentStack->count - 1)
        {
            /* Set error. */
            if (glmIS_SUCCESS(context->error))
            {
                glmERROR(GL_STACK_OVERFLOW);
            }
        }
        else
        {
            /* Copy the previous matrix. */
            context->currentStack->topMatrix[1]
                = context->currentStack->topMatrix[0];

            /* Increment the stack pointer. */
            context->currentStack->index++;
            context->currentStack->topMatrix++;
            context->currentMatrix++;

            /* Notify dependents of the change. */
            (*context->currentStack->currChanged) (context);
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glPopMatrix(
    void
    )
{
    glmENTER()
    {
		gcmDUMP_API("${ES11 glPopMatrix}");

		glmPROFILE(context, GLES1_POPMATRIX, 0);
        /* Already on the first matrix? */
        if (context->currentStack->index == 0)
        {
            /* Set error. */
            if (glmIS_SUCCESS(context->error))
            {
                glmERROR(GL_STACK_UNDERFLOW);
            }
        }
        else
        {
            /* Decrement the stack pointer. */
            context->currentStack->index--;
            context->currentStack->topMatrix--;
            context->currentMatrix--;

            /* Notify dependents of the change. */
            (*context->currentStack->currChanged) (context);
            (*context->currentStack->dataChanged) (context);
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glCurrentPaletteMatrixOES (GL_OES_matrix_palette)
**
**  glCurrentPaletteMatrixOES defines which of the palette's matrices is
**  affected by subsequent matrix operations when the current matrix mode is
**  MATRIX_PALETTE_OES. glCurrentPaletteMatrixOES generates the error
**  INVALID_VALUE if the <MatrixIndex> parameter is not between 0 and
**  MAX_PALETTE_MATRICES_OES - 1.
**
**  INPUT:
**
**      MatrixIndex
**          Index of the palette matrix to set as current.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_EXTENTION

GL_API void GL_APIENTRY glCurrentPaletteMatrixOES(
    GLuint MatrixIndex
    )
{
    glmENTER1(glmARGUINT, MatrixIndex)
    {
		gcmDUMP_API("${ES11 glCurrentPaletteMatrixOES 0x%08X}", MatrixIndex);

		if (MatrixIndex >= glvMAX_PALETTE_MATRICES)
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Set the new current. */
        context->currentPalette = MatrixIndex;

        /* Are we currently in palette matrix mode? */
        if ((context->matrixMode >= glvPALETTE_MATRIX_0) &&
#if glvMAX_PALETTE_MATRICES > 9
            (context->matrixMode <= glvPALETTE_MATRIX_31))
#else
            (context->matrixMode <= glvPALETTE_MATRIX_8))
#endif
        {
            /* Update matrix mode. */
            context->matrixMode
                = (gleMATRIXMODE) (glvPALETTE_MATRIX_0
                + context->currentPalette);

            /* Update the stack. */
            context->currentStack = &context->matrixStackArray[context->matrixMode];
            context->currentMatrix = context->currentStack->topMatrix;
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glLoadPaletteFromModelViewMatrixOES (GL_OES_matrix_palette)
**
**  glLoadPaletteFromModelViewMatrixOES copies the current model view matrix
**  to a matrix in the matrix palette, specified by CurrentPaletteMatrixOES.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glLoadPaletteFromModelViewMatrixOES(
    void
    )
{
    glmENTER()
    {
        gctUINT index;
        glsMATRIX_PTR matrix;

		gcmDUMP_API("${ES11 glLoadPaletteFromModelViewMatrixOES}");

		/* Determine the stack index. */
        index = glvPALETTE_MATRIX_0 + context->currentPalette;

        /* Get the top matrix. */
        matrix = context->matrixStackArray[index].topMatrix;

        /* Copy the current Model View. */
        gcmVERIFY_OK(gcoOS_MemCopy(
            matrix, context->modelViewMatrix, gcmSIZEOF(glsMATRIX)
            ));

        /* Notify of the change. */
        (*context->matrixStackArray[index].dataChanged) (context);
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glQueryMatrixxOES (GL_OES_query_matrix)
**
**  glQueryMatrixxOES returns the values of the current matrix. Mantissa returns
**  the 16 mantissa values of the current matrix, and Exponent returns the
**  correspnding 16 exponent values. The matrix value i is then close to
**  Mantissa[i] * 2 ** Exponent[i].
**
**  Use glMatrixMode and glActiveTexture, to select the desired matrix to
**  return.
**
**  If all are valid (not NaN or Inf), glQueryMatrixxOES returns the status
**  value 0. Otherwise, for every component i which is not valid, the ith bit
**  is set.
**
**  The implementation is not required to keep track of overflows. If overflows
**  are not tracked, the returned status value is always 0.
**
**  INPUT:
**
**      Mantissa
**          Returns the mantissi of the current matrix.
**
**      Exponent
**          Returns the exponents of the current matrix.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API GLbitfield GL_APIENTRY glQueryMatrixxOES(
    GLfixed* Mantissa,
    GLint* Exponent
    )
{
    GLbitfield valid = 0;

    glmENTER2(glmARGPTR, Mantissa, glmARGPTR, Exponent)
    {
        switch (context->currentMatrix->type)
        {
        case glvFIXED:
            gcoOS_MemCopy(
                Mantissa,
                context->currentMatrix->value,
                16 * sizeof(GLfixed)
                );
            gcoOS_ZeroMemory(
                Exponent,
                16 * sizeof(GLfixed)
                );
            break;

#ifndef COMMON_LITE
        case glvFLOAT:
            {
                int i;

                for (i = 0; i < 16; i++)
                {
                    GLbitfield bits = context->currentMatrix->value[i].i;
                    GLfloat m = context->currentMatrix->value[i].f;

                    if ((bits & 0x7f800000) == 0x7f800000)
                    {
                        /* IEEE754: INF, NaN, non-regular float value */
                        valid |= (1 << i);

                        continue;
                    }

                    Exponent[i] = 0;

                    while (gcmABS(m) >= 32768.0f)
                    {
                        Exponent[i]++;
                        m /= 2.0f;
                    }

                    Mantissa[i] = (GLfixed) (m * 65536.0f);
                }
            }
            break;
#endif

        default:
            gcmFATAL("Invalid matrix type: %d", context->currentMatrix->type);
        }

		gcmDUMP_API("${ES11 glQueryMatrixxOES (0x%08X) (0x%08X)", Mantissa, Exponent);
		gcmDUMP_API_ARRAY(Mantissa, 16);
		gcmDUMP_API_ARRAY(Exponent, 16);
		gcmDUMP_API("$}");
    }
    glmLEAVE();

    return valid;
}

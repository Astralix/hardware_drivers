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




#ifndef __gc_vg_matrix_h__
#define __gc_vg_matrix_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*------------------------ Internal matrix functions. ------------------------*/

void vgfInitializeMatrixSet(
	IN vgsCONTEXT_PTR Context
	);

vgsMATRIX_PTR vgfGetIdentity(
	void
	);

gctBOOL vgfIsAffine(
	IN const vgsMATRIX_PTR Matrix
	);

gctBOOL vgfIsIdentity(
	IN OUT vgsMATRIX_PTR Matrix
	);

vgmINLINE void vgfInvalidateMatrix(
	IN OUT vgsMATRIX_PTR Matrix
	);

vgmINLINE void vgfInvalidateContainer(
	IN vgsCONTEXT_PTR Context,
	IN OUT vgsMATRIXCONTAINER_PTR Container
	);

gctFLOAT vgfGetDeterminant(
	IN OUT vgsMATRIX_PTR Matrix
	);

gctBOOL vgfInvertMatrix(
	IN const vgsMATRIX_PTR Matrix,
	OUT vgsMATRIX_PTR Result
	);

void vgfMultiplyVector2ByMatrix2x2(
	IN const vgtFLOATVECTOR2 Vector,
	IN const vgsMATRIX_PTR Matrix,
	OUT vgtFLOATVECTOR2 Result
	);

void vgfMultiplyVector2ByMatrix3x2(
	IN const vgtFLOATVECTOR2 Vector,
	IN const vgsMATRIX_PTR Matrix,
	OUT vgtFLOATVECTOR2 Result
	);

void vgfMultiplyVector3ByMatrix3x3(
	IN const vgtFLOATVECTOR3 Vector,
	IN const vgsMATRIX_PTR Matrix,
	OUT vgtFLOATVECTOR3 Result
	);

void vgfMultiplyMatrix3x3(
	IN const vgsMATRIX_PTR Matrix1,
	IN const vgsMATRIX_PTR Matrix2,
	OUT vgsMATRIX_PTR Result
	);

gctFIXED_POINT sinx(
	 gctFIXED_POINT angle
	 );

gctFIXED_POINT cosx(
	 gctFIXED_POINT angle
	 );

#ifdef __cplusplus
}
#endif

#endif

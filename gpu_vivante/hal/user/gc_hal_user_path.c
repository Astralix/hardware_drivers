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






#include "gc_hal_user_precomp.h"

#if gcdUSE_VG

#include "gc_hal_user_vg.h"

/******************************************************************************/
/** @ingroup gcoPATH
**
**	@brief	 The gcoPATH object constructor.
**
**	gcoPATH_Construct constructs a new gcoPATH object.
**
**	@param[in]	Vg		Pointer to a gcoVG object.
**	@param[out]	Path	Pointer to a variable receiving the gcoPATH object
**						pointer.
*/
gceSTATUS
gcoPATH_Construct(
	IN gcoVG Vg,
	OUT gcoPATH * Path
	)
{
	return gcvSTATUS_NOT_SUPPORTED;
}

/******************************************************************************/
/** @ingroup gcoPATH
**
**	@brief	 The gcoPATH object destructor.
**
**	gcoPATH_Destroy constructs a new gcoPATH object.
**
**	@param[in]	Path	Pointer to the gcoPATH object that needs to be
**						destructed.
*/
gceSTATUS
gcoPATH_Destroy(
	IN gcoPATH Path
	)
{
	return gcvSTATUS_NOT_SUPPORTED;
}

/******************************************************************************/
/** @ingroup gcoPATH
**
**	@brief	 Clear a gcoPATH object.
**
**	All the collected scanline data inside the gcoPATH object will be deleted.
**
**	@param[in]	Path	Pointer to the gcoPATH object.
*/
gceSTATUS
gcoPATH_Clear(
	IN gcoPATH Path
	)
{
	return gcvSTATUS_NOT_SUPPORTED;
}

/******************************************************************************/
/** @ingroup gcoPATH
**
**	@brief	 Reserve data insode a gcoPATH object.
**
**	Reserve data for a number of scanlines inside a gcoPATH object.  The
**	returned buffer can actually be smaller than requested due to hardware
**	limitations in the buffer size.
**
**	@param[in]	Path		Pointer to the gcoPATH object.
**	@param[in]	Scanlines	The number of scanlines to reserve.
**	@param[out]	Buffer		A pointer to a variable receiving the buffer where
**							the scanlines can be stored in.
**	@param[out]	BufferCount	A pointer to a variable receiving the number of
**							scanlines reserved in the Buffer.  This can actually
**							be lower than the requested number.
*/
gceSTATUS
gcoPATH_Reserve(
	IN gcoPATH Path,
	IN gctSIZE_T Scanlines,
	OUT gcsSCANLINE_PTR * Buffer,
	OUT gctSIZE_T * BufferCount
	)
{
	return gcvSTATUS_NOT_SUPPORTED;
}

/******************************************************************************/
/** @ingroup gcoPATH
**
**	@brief	 Add a number of scanlines to the gcoPATH object.
**
**	The current buffers will be grown and more buffers will be allocated to save
**	the given scanlines into the gcoPATH object.
**
**	@param[in]	Path		Pointer to the gcoPATH object.
**	@param[in]	Scanlines	The number of scanlines to add.
**	@param[in]	Buffer		A pointer to the scanline buffer to add.
*/
gceSTATUS
gcoPATH_AddScanlines(
	IN gcoPATH Path,
	IN gctSIZE_T Scanlines,
	IN gcsSCANLINE_PTR Buffer
	)
{
	return gcvSTATUS_NOT_SUPPORTED;
}

#endif /* gcdUSE_VG */

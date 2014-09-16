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


#include "gc_vgsh_precomp.h"

static const _VGTesstype   cosine[] =
{
	0.707106781186548f,
	0.923879532511287f,
	0.98078528040323f,
	0.995184726672197f,
	0.998795456205172f,
	0.999698818696204f,
	0.999924701839145f,
	0.999981175282601f
};

static const float CIRCLE_SIN[360] =
{
	0.00000000f, 0.01745241f, 0.03489950f, 0.05233596f, 0.06975647f, 0.08715574f, 0.10452846f, 0.12186935f, 0.13917311f, 0.15643448f, 0.17364819f, 0.19080900f, 0.20791170f, 0.22495106f, 0.24192190f, 0.25881904f, 0.27563736f, 0.29237172f, 0.30901700f, 0.32556814f, 0.34202015f, 0.35836795f, 0.37460658f, 0.39073116f, 0.40673664f, 0.42261827f, 0.43837118f, 0.45399052f, 0.46947157f, 0.48480961f, 0.50000000f, 0.51503813f, 0.52991927f, 0.54463905f, 0.55919296f, 0.57357645f,
	0.58778524f, 0.60181504f, 0.61566150f, 0.62932038f, 0.64278764f, 0.65605903f, 0.66913062f, 0.68199837f, 0.69465840f, 0.70710677f, 0.71933985f, 0.73135370f, 0.74314487f, 0.75470960f, 0.76604444f, 0.77714598f, 0.78801078f, 0.79863554f, 0.80901700f, 0.81915206f, 0.82903755f, 0.83867061f, 0.84804809f, 0.85716730f, 0.86602545f, 0.87461972f, 0.88294762f, 0.89100653f, 0.89879405f, 0.90630782f, 0.91354549f, 0.92050487f, 0.92718387f, 0.93358046f, 0.93969262f, 0.94551855f,
	0.95105654f, 0.95630479f, 0.96126169f, 0.96592581f, 0.97029573f, 0.97437006f, 0.97814763f, 0.98162717f, 0.98480779f, 0.98768836f, 0.99026805f, 0.99254614f, 0.99452192f, 0.99619472f, 0.99756408f, 0.99862951f, 0.99939084f, 0.99984771f, 1.00000000f, 0.99984771f, 0.99939084f, 0.99862951f, 0.99756408f, 0.99619472f, 0.99452192f, 0.99254614f, 0.99026805f, 0.98768836f, 0.98480773f, 0.98162717f, 0.97814757f, 0.97437006f, 0.97029573f, 0.96592581f, 0.96126169f, 0.95630473f,
	0.95105648f, 0.94551855f, 0.93969262f, 0.93358046f, 0.92718387f, 0.92050487f, 0.91354543f, 0.90630776f, 0.89879405f, 0.89100647f, 0.88294756f, 0.87461960f, 0.86602539f, 0.85716730f, 0.84804803f, 0.83867055f, 0.82903749f, 0.81915200f, 0.80901700f, 0.79863548f, 0.78801078f, 0.77714586f, 0.76604444f, 0.75470948f, 0.74314481f, 0.73135370f, 0.71933973f, 0.70710677f, 0.69465828f, 0.68199831f, 0.66913044f, 0.65605897f, 0.64278764f, 0.62932026f, 0.61566144f, 0.60181487f,
	0.58778518f, 0.57357645f, 0.55919284f, 0.54463905f, 0.52991915f, 0.51503801f, 0.50000006f, 0.48480955f, 0.46947157f, 0.45399037f, 0.43837112f, 0.42261809f, 0.40673658f, 0.39073116f, 0.37460649f, 0.35836792f, 0.34202000f, 0.32556811f, 0.30901679f, 0.29237160f, 0.27563736f, 0.25881892f, 0.24192186f, 0.22495088f, 0.20791161f, 0.19080901f, 0.17364806f, 0.15643445f, 0.13917294f, 0.12186928f, 0.10452849f, 0.08715564f, 0.06975647f, 0.05233581f, 0.03489945f, 0.01745222f,
	-0.00000009f, -0.01745239f, -0.03489963f, -0.05233599f, -0.06975664f, -0.08715581f, -0.10452867f, -0.12186945f, -0.13917311f, -0.15643461f, -0.17364822f, -0.19080919f, -0.20791179f, -0.22495104f, -0.24192202f, -0.25881907f, -0.27563754f, -0.29237178f, -0.30901697f, -0.32556826f, -0.34202015f, -0.35836810f, -0.37460664f, -0.39073130f, -0.40673673f, -0.42261827f, -0.43837127f, -0.45399055f, -0.46947172f, -0.48480970f, -0.50000018f, -0.51503819f, -0.52991927f, -0.54463917f, -0.55919296f, -0.57357663f,
	-0.58778536f, -0.60181504f, -0.61566156f, -0.62932044f, -0.64278775f, -0.65605909f, -0.66913062f, -0.68199843f, -0.69465840f, -0.70710689f, -0.71933985f, -0.73135382f, -0.74314487f, -0.75470960f, -0.76604456f, -0.77714598f, -0.78801072f, -0.79863572f, -0.80901712f, -0.81915212f, -0.82903761f, -0.83867055f, -0.84804827f, -0.85716742f, -0.86602545f, -0.87461972f, -0.88294756f, -0.89100665f, -0.89879411f, -0.90630782f, -0.91354543f, -0.92050481f, -0.92718399f, -0.93358052f, -0.93969268f, -0.94551855f,
	-0.95105648f, -0.95630485f, -0.96126175f, -0.96592587f, -0.97029573f, -0.97437012f, -0.97814763f, -0.98162723f, -0.98480779f, -0.98768830f, -0.99026811f, -0.99254620f, -0.99452192f, -0.99619472f, -0.99756402f, -0.99862957f, -0.99939084f, -0.99984771f, -1.00000000f, -0.99984771f, -0.99939084f, -0.99862951f, -0.99756402f, -0.99619472f, -0.99452186f, -0.99254614f, -0.99026805f, -0.98768830f, -0.98480773f, -0.98162711f, -0.97814757f, -0.97437000f, -0.97029573f, -0.96592581f, -0.96126163f, -0.95630467f,
	-0.95105648f, -0.94551855f, -0.93969262f, -0.93358034f, -0.92718375f, -0.92050481f, -0.91354543f, -0.90630782f, -0.89879394f, -0.89100641f, -0.88294756f, -0.87461972f, -0.86602545f, -0.85716712f, -0.84804797f, -0.83867049f, -0.82903761f, -0.81915182f, -0.80901682f, -0.79863542f, -0.78801072f, -0.77714598f, -0.76604420f, -0.75470942f, -0.74314475f, -0.73135364f, -0.71933985f, -0.70710653f, -0.69465822f, -0.68199825f, -0.66913056f, -0.65605909f, -0.64278740f, -0.62932020f, -0.61566138f, -0.60181499f,
	-0.58778495f, -0.57357621f, -0.55919272f, -0.54463893f, -0.52991927f, -0.51503778f, -0.49999976f, -0.48480946f, -0.46947148f, -0.45399052f, -0.43837082f, -0.42261803f, -0.40673649f, -0.39073107f, -0.37460664f, -0.35836762f, -0.34201992f, -0.32556802f, -0.30901694f, -0.29237175f, -0.27563703f, -0.25881884f, -0.24192177f, -0.22495103f, -0.20791176f, -0.19080870f, -0.17364797f, -0.15643436f, -0.13917309f, -0.12186895f, -0.10452817f, -0.08715555f, -0.06975638f, -0.05233596f, -0.03489912f, -0.01745213f,
};

static const float CIRCLE_COS[360] =
{
	1.00000000f, 0.99984771f, 0.99939084f, 0.99862951f, 0.99756408f, 0.99619472f, 0.99452192f, 0.99254614f, 0.99026805f, 0.98768836f, 0.98480773f, 0.98162717f, 0.97814763f, 0.97437006f, 0.97029573f, 0.96592581f, 0.96126169f, 0.95630473f, 0.95105654f, 0.94551855f, 0.93969262f, 0.93358040f, 0.92718387f, 0.92050487f, 0.91354543f, 0.90630776f, 0.89879405f, 0.89100653f, 0.88294756f, 0.87461972f, 0.86602539f, 0.85716730f, 0.84804809f, 0.83867055f, 0.82903755f, 0.81915206f,
	0.80901700f, 0.79863548f, 0.78801078f, 0.77714592f, 0.76604444f, 0.75470960f, 0.74314481f, 0.73135370f, 0.71933979f, 0.70710677f, 0.69465834f, 0.68199837f, 0.66913056f, 0.65605897f, 0.64278764f, 0.62932038f, 0.61566144f, 0.60181504f, 0.58778524f, 0.57357639f, 0.55919290f, 0.54463899f, 0.52991927f, 0.51503807f, 0.49999997f, 0.48480958f, 0.46947151f, 0.45399052f, 0.43837115f, 0.42261824f, 0.40673661f, 0.39073107f, 0.37460652f, 0.35836786f, 0.34202015f, 0.32556814f,
	0.30901697f, 0.29237166f, 0.27563727f, 0.25881907f, 0.24192190f, 0.22495104f, 0.20791166f, 0.19080894f, 0.17364810f, 0.15643437f, 0.13917311f, 0.12186933f, 0.10452842f, 0.08715568f, 0.06975640f, 0.05233597f, 0.03489950f, 0.01745238f, -0.00000004f, -0.01745247f, -0.03489958f, -0.05233606f, -0.06975648f, -0.08715577f, -0.10452851f, -0.12186941f, -0.13917319f, -0.15643445f, -0.17364819f, -0.19080903f, -0.20791174f, -0.22495112f, -0.24192198f, -0.25881916f, -0.27563736f, -0.29237175f,
	-0.30901703f, -0.32556823f, -0.34202024f, -0.35836795f, -0.37460661f, -0.39073116f, -0.40673670f, -0.42261833f, -0.43837112f, -0.45399061f, -0.46947157f, -0.48480976f, -0.50000006f, -0.51503801f, -0.52991933f, -0.54463905f, -0.55919302f, -0.57357645f, -0.58778518f, -0.60181510f, -0.61566144f, -0.62932050f, -0.64278764f, -0.65605915f, -0.66913068f, -0.68199831f, -0.69465846f, -0.70710677f, -0.71933991f, -0.73135376f, -0.74314493f, -0.75470966f, -0.76604444f, -0.77714604f, -0.78801078f, -0.79863560f,
	-0.80901706f, -0.81915206f, -0.82903761f, -0.83867055f, -0.84804815f, -0.85716730f, -0.86602539f, -0.87461978f, -0.88294756f, -0.89100659f, -0.89879405f, -0.90630788f, -0.91354549f, -0.92050487f, -0.92718393f, -0.93358046f, -0.93969268f, -0.94551861f, -0.95105660f, -0.95630479f, -0.96126169f, -0.96592587f, -0.97029573f, -0.97437012f, -0.97814763f, -0.98162717f, -0.98480779f, -0.98768836f, -0.99026811f, -0.99254614f, -0.99452192f, -0.99619472f, -0.99756408f, -0.99862957f, -0.99939084f, -0.99984771f,
	-1.00000000f, -0.99984771f, -0.99939084f, -0.99862951f, -0.99756402f, -0.99619472f, -0.99452186f, -0.99254614f, -0.99026805f, -0.98768830f, -0.98480773f, -0.98162717f, -0.97814757f, -0.97437006f, -0.97029567f, -0.96592581f, -0.96126163f, -0.95630473f, -0.95105654f, -0.94551855f, -0.93969262f, -0.93358040f, -0.92718381f, -0.92050475f, -0.91354543f, -0.90630782f, -0.89879400f, -0.89100653f, -0.88294750f, -0.87461966f, -0.86602527f, -0.85716724f, -0.84804809f, -0.83867049f, -0.82903755f, -0.81915194f,
	-0.80901694f, -0.79863548f, -0.78801066f, -0.77714592f, -0.76604432f, -0.75470954f, -0.74314481f, -0.73135364f, -0.71933979f, -0.70710665f, -0.69465834f, -0.68199819f, -0.66913050f, -0.65605903f, -0.64278752f, -0.62932032f, -0.61566150f, -0.60181475f, -0.58778507f, -0.57357633f, -0.55919290f, -0.54463911f, -0.52991897f, -0.51503789f, -0.49999991f, -0.48480961f, -0.46947163f, -0.45399022f, -0.43837097f, -0.42261818f, -0.40673664f, -0.39073122f, -0.37460634f, -0.35836777f, -0.34202006f, -0.32556817f,
	-0.30901709f, -0.29237145f, -0.27563721f, -0.25881898f, -0.24192193f, -0.22495072f, -0.20791145f, -0.19080885f, -0.17364813f, -0.15643452f, -0.13917278f, -0.12186912f, -0.10452834f, -0.08715571f, -0.06975655f, -0.05233565f, -0.03489929f, -0.01745230f, 0.00000001f, 0.01745232f, 0.03489979f, 0.05233615f, 0.06975657f, 0.08715574f, 0.10452884f, 0.12186962f, 0.13917327f, 0.15643454f, 0.17364815f, 0.19080934f, 0.20791194f, 0.22495121f, 0.24192195f, 0.25881901f, 0.27563769f, 0.29237193f,
	0.30901712f, 0.32556820f, 0.34202009f, 0.35836825f, 0.37460679f, 0.39073125f, 0.40673667f, 0.42261818f, 0.43837142f, 0.45399067f, 0.46947166f, 0.48480964f, 0.49999991f, 0.51503831f, 0.52991945f, 0.54463911f, 0.55919290f, 0.57357675f, 0.58778548f, 0.60181516f, 0.61566150f, 0.62932038f, 0.64278787f, 0.65605921f, 0.66913074f, 0.68199837f, 0.69465834f, 0.70710701f, 0.71933997f, 0.73135382f, 0.74314487f, 0.75470954f, 0.76604462f, 0.77714610f, 0.78801084f, 0.79863554f,
	0.80901724f, 0.81915224f, 0.82903767f, 0.83867061f, 0.84804809f, 0.85716748f, 0.86602557f, 0.87461978f, 0.88294762f, 0.89100653f, 0.89879423f, 0.90630788f, 0.91354555f, 0.92050487f, 0.92718387f, 0.93358058f, 0.93969268f, 0.94551861f, 0.95105654f, 0.95630473f, 0.96126181f, 0.96592587f, 0.97029579f, 0.97437006f, 0.97814757f, 0.98162723f, 0.98480779f, 0.98768836f, 0.99026805f, 0.99254620f, 0.99452192f, 0.99619472f, 0.99756408f, 0.99862951f, 0.99939084f, 0.99984771f,
};

#if LOCAL_MEM_OPTIM
gcmMEM_DeclareAFSMemPool(gctINT32, Int, ,);
gcmMEM_DeclareAFSMemPool(gctINT32*, Intp, ,);
#endif

/*******************************************************************************
**
**	_Diag
**  Compute the diagonal of two edges.
**
**	INPUT:
**      x, y: The two edges.
**
**	OUTPUT:
**      The length of the diagonal.
**
*/
_VGTesstype _Diag(
    _VGTesstype x,
    _VGTesstype y
    )
{
	_VGTesstype r;

	r = (_VGTesstype)gcoMATH_SquareRoot(x*x + y*y);

	return r;
}

_VGTesstype GetLineDirection(_VGVector2 *p0, _VGVector2 *p1, _VGVector2 *tanVal)
{
    _VGTesstype dx, dy, dist;

	dx = p1->x - p0->x;
	dy = p1->y - p0->y;
	dist = (_VGTesstype)TESS_DIAG(dx, dy);
    if (dist > 0)
    {
        tanVal->x  = TESS_DIV(dx, dist);
        tanVal->y  = TESS_DIV(dy, dist);
    }
    else
    {
        tanVal->x  = TESS_CONST_ONE;
        tanVal->y  = 0;
    }

	return dist;
}

void    _SetEpsilonScale(
	_VGTessellationContext	*tContext,
    _VGTesstype scale
    )
{
	_VGTesstype inv;

	if (scale < 1.0f)
	{
		inv = TESS_DIV(TESS_CONST_ONE, scale);
	}
	else
	{
		inv = TESS_DIV(TESS_CONST_ONE, (scale - 1.0f + 2.0f) / 2.0f);
	}
    tContext->epsilon = TESS_MUL(EPSILON, inv);
	tContext->eBezierThold = inv * inv;
    tContext->epsilonSquare = EPSILON_SQUARE * tContext->eBezierThold;
	if (scale > 1)
		tContext->flattenForStroke = gcvTRUE;
}

void    _SetStrokeFlatten(
	_VGTessellationContext	*tContext,
	_VGTesstype    strokeWidth
    )
{
	if (strokeWidth > 1)
    {   /* Actually we need a precise formula here to compute the angle epsilon. */
        tContext->angleEpsilon = ANGLE_EPSILON2;
		tContext->flattenForStroke = gcvTRUE;
    }
	else
		tContext->flattenForStroke = gcvFALSE;
}

/*******************************************************************************
**
**	GetPathLength
**  Compute the length of a specific sub-path.
**
**	INPUT:
**      *p is the given path object;
**      start is the begining segment;
**      num is the number of segments.
**
**	OUTPUT:
**      The length of the sub-path.
**
*/
_VGTesstype GetPathLength(
    _VGPath *p,
    VGint   start,
    VGint   num
    )
{
    _VGTesstype     result = 0;
    _VGPathSegInfo  *startSeg, *endSeg;

	gcmHEADER_ARG("p=0x%x start=%d num=%d", p, start, num);

    startSeg = p->segmentsInfo.items + start;
    endSeg = p->segmentsInfo.items + start + num;
    while(startSeg < endSeg)
    {
        result += startSeg->length;
        ++startSeg;
    }


	gcmFOOTER_ARG("return=%f", result);

    return result;
}

/*******************************************************************************
**
**	GetPointAlongPath
**  Compute a point on the path by giving a length value.
**
**	INPUT:
**      *p              is the given path object;
**      startSegment    is the begining segment;
**      numSegments     is the number of segments.
**      distance        is the distance from the begining to the given point.
**
**	OUTPUT:
**      *x, *y          are the coordinates of the point;
**      *tangentX, *tangentY are the tangent values of the point.
**
*/

void    GetPointAlongPath(
    _VGPath     *p,
    gctINT32    startSegment,
    gctINT32    numSegments,
    _VGTesstype distance,
    _VGTesstype *x,
    _VGTesstype *y,
    _VGTesstype *tangentX,
    _VGTesstype *tangentY
    )
{
    gctINT32     j, i;
    _VGTessPoint    *point;
    _VGPathSegInfo  *segStart, *segEnd, *segIt;

	gcmHEADER_ARG("p=0x%x startSegment=%d numSegments=%d distance=%f x=0x%x y=0x%x tangentX=0x%x tangentY=0x%x",
		p, startSegment, numSegments, distance, x, y, tangentX, tangentY);

	/* Initialization default. */
	if (x != gcvNULL)
	{
		*x = 0.0f;
	}
	if (y != gcvNULL)
	{
		*y = 0.0f;
	}
	if (tangentX != gcvNULL)
	{
		*tangentX = 0.0f;
	}
	if (tangentY != gcvNULL)
	{
		*tangentY = 0.0f;
	}

	segStart = p->segmentsInfo.items + startSegment;

	/* VG RI11 says to ignore the MOVE_TOs (both in the beginning and in the end). */
	while (numSegments > 0 && segStart->segShape.type == VG_MOVE_TO)
	{
		segStart++;
		numSegments--;
		startSegment++;
	}
	while (numSegments > 0 && (segStart + numSegments - 1)->segShape.type == VG_MOVE_TO)
		numSegments--;

	segEnd = segStart + gcmMIN(numSegments, p->segments.size - startSegment);
	segIt = segStart;
	/* VG RI11 says if the sub path's length is 0, return default. */
	while (segIt < segEnd)
	{
		if (segIt->length > 0)
			break;
		segIt++;
	}
	if (segIt >= segEnd)	/* zero-length subpath */
	{
		point = p->tessellateResult.flattenedStrokePath.points + segStart->startPoint;
		if (x != gcvNULL)
			*x = point->coord.x;
		if (y != gcvNULL)
			*y = point->coord.y;
		if (tangentX != gcvNULL)
			*tangentX = 1.0f;
		if (tangentY != gcvNULL)
			*tangentY = 0.0f;

		gcmFOOTER_NO();

		return;
	}

    if (distance <= 0)
    {
		/* If it has tangents (not a cusp point), return it. */
		if (segIt->inTan.x != UN_INIT_TAN_VALUE)
		{
			if (x != gcvNULL)
				*x = segIt->segShape.fromP->x;
			if (y != gcvNULL)
				*y = segIt->segShape.fromP->y;
			if (tangentX != gcvNULL)
				*tangentX = segIt->inTan.x;
			if (tangentY != gcvNULL)
				*tangentY = segIt->inTan.y;
		}
		/* Otherwise, Search backwards to find a tangent as its incoming to return. */
		else
		{
			while (segIt >= p->segmentsInfo.items)
			{
				if (segIt->outTan.x != UN_INIT_TAN_VALUE)
				{
					if (x != gcvNULL)
						*x = segIt->segShape.toP->x;
					if (y != gcvNULL)
						*y = segIt->segShape.toP->y;
					if (tangentX != gcvNULL)
						*tangentX = segIt->outTan.x;
					if (tangentY != gcvNULL)
						*tangentY = segIt->outTan.y;

					break;
				}

				segIt--;
			}

			if (segIt < p->segmentsInfo.items)		/* Not found, use default values. */
			{
				if (x != gcvNULL)
					*x = p->segmentsInfo.items->segShape.toP->x;
				if (y != gcvNULL)
					*y = p->segmentsInfo.items->segShape.toP->y;
				if (tangentX != gcvNULL)
					*tangentX = 1.0f;
				if (tangentY != gcvNULL)
					*tangentY = 0.0f;
			}
		}

		gcmFOOTER_NO();

        return;
    }

    /* Search for the path segment which the point lies on. */
	j = 0;
    while (segIt < segEnd)
    {
        if (distance <= segIt->length)
        {
            break;
        }

        distance -= segIt->length;
        ++segIt;
		++j;
    }

	if (segIt >= segEnd)	/* Not found a segment can provide tangents */
	{
		segIt = segEnd - 1;
		/* If it has tangents (not a cusp point), return it. */
		if (segIt->outTan.x != UN_INIT_TAN_VALUE)
		{
			if (x != gcvNULL)
				*x = segIt->segShape.toP->x;
			if (y != gcvNULL)
				*y = segIt->segShape.toP->y;
			if (tangentX != gcvNULL)
				*tangentX = segIt->outTan.x;
			if (tangentY != gcvNULL)
				*tangentY = segIt->outTan.y;
		}
		else
		{
			/* Search forward to find a visual end. */
			segIt = segEnd;
			while (segIt < p->segmentsInfo.items + p->segmentsInfo.size / sizeof(_VGPathSegInfo))
			{
				if (segIt->inTan.x != UN_INIT_TAN_VALUE)
				{
					if (x != gcvNULL)
						*x = segIt->segShape.fromP->x;
					if (y != gcvNULL)
						*y = segIt->segShape.fromP->y;
					if (tangentX != gcvNULL)
						*tangentX = segIt->inTan.x;
					if (tangentY != gcvNULL)
						*tangentY = segIt->inTan.y;
					break;
				}
				segIt++;
			}
			/* If not found. */
			if (segIt >= p->segmentsInfo.items + p->segmentsInfo.size / sizeof(_VGPathSegInfo))
			{
				segIt = segEnd - 1;

				/* Search backwards to find a segment to provide tangents. */
				while (segIt >= p->segmentsInfo.items)
				{
					if (segIt->outTan.x != UN_INIT_TAN_VALUE)
					{
						if (x != gcvNULL)
							*x = segIt->segShape.toP->x;
						if (y != gcvNULL)
							*y = segIt->segShape.toP->y;
						if (tangentX != gcvNULL)
							*tangentX = segIt->outTan.x;
						if (tangentY != gcvNULL)
							*tangentY = segIt->outTan.y;
						break;
					}

					segIt--;
				}

				if (segIt < p->segmentsInfo.items)		/* Not found, use default values. */
				{
					if (x != gcvNULL)
						*x = segIt->segShape.toP->x;
					if (y != gcvNULL)
						*y = segIt->segShape.toP->y;
					if (tangentX != gcvNULL)
						*tangentX = 1.0f;
					if (tangentY != gcvNULL)
						*tangentY = 0.0f;
				}
			}
		}

		gcmFOOTER_NO();

		return;
	}
	else
	{
		segStart = segIt;
		/* Search the point */
		point = p->tessellateResult.flattenedStrokePath.points + segStart->startPoint;
		j = segStart->numPoints - 1;
		for (i = 0; i < j; ++i)
		{
			if (distance <= point->distance2Next)
			{
				if (point->flags == POINT_FLAT)
				{
					if (tangentX != gcvNULL)
						*tangentX = point->outTan.x;
					if (tangentY != gcvNULL)
						*tangentY = point->outTan.y;
					if (x != gcvNULL)
						*x = point->coord.x + point->outTan.x * distance;
					if (y != gcvNULL)
						*y = point->coord.y + point->outTan.y * distance;
				}
				else
				{
					_VGVector2 tan0, tan1;
					if (point == p->tessellateResult.flattenedStrokePath.points + segStart->startPoint)
					{
						tan0.x = segStart->inTan.x;
						tan0.y = segStart->inTan.y;
						if (segStart->numPoints == 2)
						{
							tan1.x = segStart->outTan.x;
							tan1.y = segStart->outTan.y;
						}
						else
						{
							tan1.x = ((point + 1)->inTan.x + (point + 1)->outTan.x) / 2;
							tan1.y = ((point + 1)->inTan.y + (point + 1)->outTan.y) / 2;
						}
					}
					else
					{
						tan0.x = (point->inTan.x + point->outTan.x) / 2;
						tan0.y = (point->inTan.y + point->outTan.y) / 2;
						tan1.x = ((point + 1)->inTan.x + (point + 1)->outTan.x) / 2;
						tan1.y = ((point + 1)->inTan.y + (point + 1)->outTan.y) / 2;
					}
					if (tangentX != gcvNULL)
						*tangentX = tan0.x * (1.0f - distance / point->distance2Next) + tan1.x * distance / point->distance2Next;
					if (tangentY != gcvNULL)
						*tangentY = tan0.y * (1.0f - distance / point->distance2Next) + tan1.y * distance / point->distance2Next;
					if (x != gcvNULL)
						*x = point->coord.x + point->outTan.x * distance;
					if (y != gcvNULL)
						*y = point->coord.y + point->outTan.y * distance;
				}
				break;
			}

			distance -= point->distance2Next;
			++point;
		}
	}

	gcmFOOTER_NO();
}

/*******************************************************************************
**
**	GetPathBounds
**  Query the bounds of a path.
**
**	INPUT:
**      *p              is the given path object;
**
**	OUTPUT:
**      *minx, *miny    are the coordinates of the lower-left point of the bounds;
**      *width, *height are the size of the bounds.
**
*/
void GetPathBounds(
	_VGContext	*context,
    _VGPath     *p,
    VGfloat     *minx,
    VGfloat     *miny,
    VGfloat     *width,
    VGfloat     *height
    )
{
	gcmHEADER_ARG("context=0x%x p=0x%x minx=0x%x miny=0x%x width=0x%x height=0x%x",
		context, p, minx, miny, width, height);

	PathDirty(p, VGTessPhase_ALL);
	if (!TessFlatten(context, p, &context->pathUserToSurface, -1.0f))
	{
		p->tessellateResult.bound.x = 0;
		p->tessellateResult.bound.y = 0;
		p->tessellateResult.bound.width = -1;
		p->tessellateResult.bound.height = -1;
	}

	*minx  = p->tessellateResult.bound.x;
    *miny  = p->tessellateResult.bound.y;
    *width = p->tessellateResult.bound.width;
    *height = p->tessellateResult.bound.height;

	gcmFOOTER_NO();
}

void	_VGTessellationContextCtor(
	gcoOS	os,
	_VGTessellationContext *tContext
	)
{
	gcmHEADER_ARG("os=0x%x tContext=0x%x",
				   os, tContext);

	/* Flattening States. */
	_ResetFlattenStates(tContext);

#if USE_TA
#define	INIT_PT_COUNT	200
	tContext->tree					= (_VGTreeNode*)TA_Init(os, INIT_PT_COUNT * sizeof(_VGTreeNode) * 8, 1);
	tContext->regions				= (_VGTrapezoid*)TA_Init(os, (INIT_PT_COUNT * 2 + 1) * sizeof(_VGTrapezoid), 1);
	tContext->pointsAdded			= (gctBOOL*)TA_Init(os, INIT_PT_COUNT * sizeof(gctBOOL), 1);
	tContext->edgeAdded				= (gctBOOL*)TA_Init(os, INIT_PT_COUNT * sizeof(gctBOOL), 1);
	tContext->edgeHigh				= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->edgeLow				= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->loopStart				= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->pointsTreePosition	= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->edgeInLengths			= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->edgeOutLengths		= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->edgeUpDown			= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->regionBelow			= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->regionBelow2			= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->regionBelow3Lengths	= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->regionAboveLengths	= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->trapezoidDepth		= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->nextEven				= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->nextOdd				= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);
	tContext->edgeDirection			= (gctINT32*)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32), 1);

	/* 2-level pointers. */
	tContext->edgeIn		= (gctINT32**)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32*), 2);
	tContext->edgeOut		= (gctINT32**)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32*), 2);
	tContext->regionBelow3	= (gctINT32**)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32*), 2);
	tContext->regionAbove	= (gctINT32**)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32*), 2);
	tContext->mountains		= (gctINT32**)TA_Init(os, INIT_PT_COUNT * sizeof(gctINT32*), 2);

	tContext->triangles = gcvNULL;
#else
	/* Triangulation States. */
    tContext->verticesLength            = 0;
    tContext->numLoops                  = 0;
    tContext->loopStart                 = 0;
    tContext->totalPoints               = 0;
    tContext->originalTotalPoints       = 0;
    tContext->treeLength                = 0;
    tContext->treeTop                   = 0;
    tContext->regionsLength             = 0;
    tContext->originalTotalPoints       = 0;
    tContext->edgeInLength              = 0;
    tContext->edgeOutLength             = 0;
    tContext->regionCounter             = 0;
    tContext->regionBelow3Length        = 0;
    tContext->regionAboveLength         = 0;

    tContext->edgeHigh = gcvNULL;
    tContext->edgeLow = gcvNULL;
    tContext->loopStart = gcvNULL;
    tContext->tree = gcvNULL;
    tContext->regions = gcvNULL;
    tContext->pointsAdded = gcvNULL;
    tContext->edgeAdded = gcvNULL;
    tContext->pointsTreePosition = gcvNULL;
    tContext->edgeInLengths = gcvNULL;
    tContext->edgeOutLengths = gcvNULL;
    tContext->edgeUpDown = gcvNULL;
    tContext->regionBelow = gcvNULL;
    tContext->regionBelow2 = gcvNULL;
    tContext->regionBelow3Lengths = gcvNULL;
    tContext->regionAboveLengths = gcvNULL;
    tContext->trapezoidDepth = gcvNULL;
    tContext->nextEven = gcvNULL;
    tContext->nextOdd = gcvNULL;
    tContext->edgeDirection = gcvNULL;
    tContext->edgeIn = gcvNULL;
    tContext->edgeOut = gcvNULL;
    tContext->regionBelow3 = gcvNULL;
    tContext->regionAbove = gcvNULL;
#endif
	tContext->errorHandling				= gcvTRUE;
	tContext->flattenForStroke			= gcvFALSE;
    tContext->triangleCounter           = 0;

#if LOCAL_MEM_OPTIM
	gcfMEM_InitAFSMemPool(&tContext->IntPool, os, 1024, sizeof(gctINT32));
	gcfMEM_InitAFSMemPool(&tContext->IntpPool, os, 1024, sizeof(gctINT32*));
#endif

	tContext->strokeScale = 1.0f;

	gcmFOOTER_NO();
}

void	_VGTessellationContextDtor(
	_VGContext	*context
	)
{
#if USE_TA
	gcoOS	os = context->os;
	_VGTessellationContext	*tContext = &context->tessContext;

	gcmHEADER_ARG("context=0x%x", context);

	if (tContext->tree != gcvNULL)
		TA_Destroy(os, (void**)&tContext->tree, 1);
	if (tContext->regions != gcvNULL)
		TA_Destroy(os, (void**)&tContext->regions, 1);
	if (tContext->pointsAdded != gcvNULL)
		TA_Destroy(os, (void**)&tContext->pointsAdded, 1);
	if (tContext->edgeAdded != gcvNULL)
		TA_Destroy(os, (void**)&tContext->edgeAdded, 1);
	if (tContext->edgeHigh != gcvNULL)
		TA_Destroy(os, (void**)&tContext->edgeHigh, 1);
	if (tContext->edgeLow != gcvNULL)
		TA_Destroy(os, (void**)&tContext->edgeLow, 1);
	if (tContext->loopStart != gcvNULL)
		TA_Destroy(os, (void**)&tContext->loopStart, 1);
	if (tContext->pointsTreePosition != gcvNULL)
		TA_Destroy(os, (void**)&tContext->pointsTreePosition, 1);
	if (tContext->edgeInLengths != gcvNULL)
		TA_Destroy(os, (void**)&tContext->edgeInLengths, 1);
	if (tContext->edgeOutLengths != gcvNULL)
		TA_Destroy(os, (void**)&tContext->edgeOutLengths, 1);
	if (tContext->edgeUpDown != gcvNULL)
		TA_Destroy(os, (void**)&tContext->edgeUpDown, 1);
	if (tContext->regionBelow != gcvNULL)
		TA_Destroy(os, (void**)&tContext->regionBelow, 1);
	if (tContext->regionBelow2 != gcvNULL)
		TA_Destroy(os, (void**)&tContext->regionBelow2, 1);
	if (tContext->regionBelow3Lengths != gcvNULL)
		TA_Destroy(os, (void**)&tContext->regionBelow3Lengths, 1);
	if (tContext->regionAboveLengths != gcvNULL)
		TA_Destroy(os, (void**)&tContext->regionAboveLengths, 1);
	if (tContext->trapezoidDepth != gcvNULL)
		TA_Destroy(os, (void**)&tContext->trapezoidDepth, 1);
	if (tContext->nextEven != gcvNULL)
		TA_Destroy(os, (void**)&tContext->nextEven, 1);
	if (tContext->nextOdd != gcvNULL)
		TA_Destroy(os, (void**)&tContext->nextOdd, 1);
	if (tContext->edgeDirection != gcvNULL)
		TA_Destroy(os, (void**)&tContext->edgeDirection, 1);

	/* 2-level pointers. */
	TA_Destroy(os, (void**)&tContext->edgeIn, 2);
	TA_Destroy(os, (void**)&tContext->edgeOut, 2);
	TA_Destroy(os, (void**)&tContext->regionBelow3, 2);
	TA_Destroy(os, (void**)&tContext->regionAbove, 2);
	TA_Destroy(os, (void**)&tContext->mountains, 2);
#else
	_VGResetTessellationContext(context);
#endif
#if LOCAL_MEM_OPTIM
	gcfMEM_FreeAFSMemPool(&tContext->IntPool);
	gcfMEM_FreeAFSMemPool(&tContext->IntpPool);
#endif

	gcmFOOTER_NO();
}

#if USE_FTA
void	_VGFlattenContextCtor(
	gcoOS os,
	_VGFlattenContext	*fContext
	)
{
#define	SEGS	100
#define	SUBS	20
	gcmHEADER_ARG("os=0x%x fContext=0x%x", os, fContext);

	fContext->numPointsInSegment = (gctINT32*)TA_Init(os, sizeof(gctINT32) * SEGS, 1);
	fContext->numSegsInSubPath = (gctINT32*)TA_Init(os, sizeof(gctINT32) * SUBS, 1);
	fContext->pointsInSegment = (_VGTessPoint**)TA_Init(os, sizeof(_VGTessPoint*) * SEGS, 2);

	gcmFOOTER_NO();
#undef	SUBS
#undef	SEGS
}

void	_VGFlattenContextDtor(
	gcoOS	os,
	_VGFlattenContext	*fContext
	)
{
	gcmHEADER_ARG("os=0x%x fContext=0x%x", os, fContext);

	TA_Destroy(os, (void**)&fContext->numPointsInSegment, 1);
	TA_Destroy(os, (void**)&fContext->numSegsInSubPath, 1);
	TA_Destroy(os, (void**)&fContext->pointsInSegment, 2);

	gcmFOOTER_NO();
}
#endif

void	_VGResetTessellationContext(
	_VGContext	*context
	)
{
	gcmHEADER_ARG("context=0x%x", context);

	_VGResetTessellationContextFlatten(&context->tessContext);
	_VGResetTessellationContextTriangulate(context);

	gcmFOOTER_NO();
}

void	_VGResetTessellationContextFlatten(
	_VGTessellationContext *tContext
	)
{
	gcmHEADER_ARG("tContext=0x%x", tContext);

	_ResetFlattenStates(tContext);

	gcmFOOTER_NO();
}

void	_VGResetTessellationContextTriangulate(
	_VGContext	*context
	)
{
	gcmHEADER_ARG("context=0x%x", context);

	_ResetTriangulateStates(context);

	gcmFOOTER_NO();
}

void _ResetFlattenStates(
	_VGTessellationContext	*tContext
	)
{
    gctINT32 i;

    for (i = 0; i < 8; ++i)
    {
        tContext->iterations[i] = 0;
    }
    tContext->iterations[8] = TESS_CONST_ONE;

    tContext->highPos = 1000;
	tContext->lowPos = 0;
}

void _ResetTriangulateStates(
	_VGContext	*context
)
{
	_VGTessellationContext	*tContext = &context->tessContext;

#if !USE_TA
    OVG_SAFE_FREE(context->os, tContext->edgeHigh);
    OVG_SAFE_FREE(context->os, tContext->edgeLow);
    OVG_SAFE_FREE(context->os, tContext->loopStart);
    OVG_SAFE_FREE(context->os, tContext->tree);
    OVG_SAFE_FREE(context->os, tContext->regions);
    OVG_SAFE_FREE(context->os, tContext->pointsAdded);
    OVG_SAFE_FREE(context->os, tContext->edgeAdded);
    OVG_SAFE_FREE(context->os, tContext->pointsTreePosition);
    OVG_SAFE_FREE(context->os, tContext->edgeInLengths);
    OVG_SAFE_FREE(context->os, tContext->edgeOutLengths);
    OVG_SAFE_FREE(context->os, tContext->edgeUpDown);
    OVG_SAFE_FREE(context->os, tContext->regionBelow);
    OVG_SAFE_FREE(context->os, tContext->regionBelow2);
    OVG_SAFE_FREE(context->os, tContext->regionBelow3Lengths);
    OVG_SAFE_FREE(context->os, tContext->regionAboveLengths);
    OVG_SAFE_FREE(context->os, tContext->trapezoidDepth);
    OVG_SAFE_FREE(context->os, tContext->nextEven);
    OVG_SAFE_FREE(context->os, tContext->nextOdd);
    OVG_SAFE_FREE(context->os, tContext->edgeDirection);

    for (i = 0; i < tContext->edgeInLength; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->edgeIn[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->edgeIn);
    for (i = 0; i < tContext->edgeOutLength; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->edgeOut[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->edgeOut);
    for (i = 0; i < tContext->regionBelow3Length; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->regionBelow3[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->regionBelow3);
    for (i = 0; i < tContext->regionAboveLength; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->regionAbove[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->regionAbove);
#endif

    tContext->verticesLength            = 0;
    tContext->numLoops                  = 0;
    tContext->totalPoints               = 0;
    tContext->originalTotalPoints       = 0;
    tContext->treeLength                = 0;
    tContext->treeTop                   = 0;
    tContext->regionsLength             = 0;
    tContext->triangleCounter           = 0;
    tContext->edgeInLength              = 0;
    tContext->edgeOutLength             = 0;
    tContext->regionCounter             = 0;
    tContext->regionBelow3Length        = 0;
    tContext->regionAboveLength         = 0;
}

gctINT32 TessellateStroke(
    _VGContext  *context,
    _VGPath     *path
    )
{
    _VGTessPoint    *points;
    gctINT32        *counts;
    gctINT32        numSubs;
    gctINT32        numTotalPoints;
    gctINT32        i, j, k, l;
    _VGTessPoint    *tempPoint, *tempPoint2;

    _VGTesstype     dashPhaseNext;
    _VGTessPoint    **dashedPoints;
    gctINT32        **dashedCounts;
    gctBOOL         isDashed;
    gctBOOL         *subPathClosed;
    _VGTesstype     lineWidth;
    void            *memStart = gcvNULL;
    VGfloat         lineW = context->strokeLineWidth;
    _VGTesstype     scale;

	_StreamPipe		streamPipe;

	gcmHEADER_ARG("context=0x%x path=0x%x", context, path);

    gcmASSERT(path->tessellateResult.flattenedStrokePath.points != gcvNULL);

    numTotalPoints = path->tessellateResult.flattenedStrokePath.numPoints;

    if (numTotalPoints < 1 || lineW < 0)
    {
		gcmFOOTER_ARG("return=%d", 0);
        return 0;
    }

    lineWidth = TESSTYPE(lineW);
    points = path->tessellateResult.flattenedStrokePath.points;
    counts = path->tessellateResult.flattenedStrokePath.numPointsInSubPath;
    numSubs = path->tessellateResult.flattenedStrokePath.numSubPaths;
    subPathClosed = path->tessellateResult.flattenedStrokePath.subPathClosed;
	scale = context->tessContext.strokeScale;
	if (scale == 0)
	{
		gcmFOOTER_ARG("return=%d", 0);
		return 0;
	}

	context->tessContext.strokeWidth = lineWidth;
	context->tessContext.strokeJoinStep = (gctINT32)(58.0f * 2.0f / (lineWidth * scale));

	if (context->tessContext.strokeJoinStep == 0)
		context->tessContext.strokeJoinStep = 1;

	if (lineWidth * scale <= 0.99f)
	{
		gctINT32 base;
		gctINT32 toMerge;
		gctFLOAT limit = 1.0f / scale;
		_VGTessPoint *tempPoints;
		gctINT32	 *tempCounts;
		gctINT32	  oCounter = 0;
		gctINT32	  merged = 0;
		base = 0;
		toMerge = 1;
		OVG_MALLOC(context->os, tempPoints, sizeof(_VGTessPoint) * numTotalPoints);
		OVG_MALLOC(context->os, tempCounts, sizeof(gctINT32) * numSubs);
		OVG_MEMCOPY(tempPoints, points, sizeof(_VGTessPoint) * numTotalPoints);
		OVG_MEMCOPY(tempCounts, counts, sizeof(gctINT32) * numSubs);
		/* Optimize point count. */
		for (k = 0; k < numSubs; ++k)
		{
			for (i = 0; i < tempCounts[k] - 2 - merged; ++i)
			{
				if (tempPoints[base + i].distance2Next < limit)
				{	/* Merge. */
					toMerge++;
					tempPoints[base + i].distance2Next = (_VGTesstype)TESS_DIAG(tempPoints[base + i].coord.x - tempPoints[toMerge].coord.x,
															   tempPoints[base + i].coord.y - tempPoints[toMerge].coord.y);
					tempPoints[base + i].outTan.x = tempPoints[toMerge].inTan.x = (tempPoints[toMerge].coord.x - tempPoints[base + i].coord.x) / tempPoints[base + i].distance2Next;
					tempPoints[base + i].outTan.y = tempPoints[toMerge].inTan.y = (tempPoints[toMerge].coord.y - tempPoints[base + i].coord.y) / tempPoints[base + i].distance2Next;
					i--;	/* Stay at the current index. */
					merged++;
				}
				else
				{	/* Move. */
					if (base + i + 1 < toMerge)	/* Ever Merged. */
					{
						tempPoints[base + i + 1] = points[toMerge];
					}
					toMerge++;
					/* Copy to buffer. */
					points[oCounter] = tempPoints[base + i];
					oCounter++;
				}
			}

			/* Copy the resut valid points of the current subpath to the buffer. */
			points[oCounter] = tempPoints[base + i];	oCounter++;
			points[oCounter] = tempPoints[toMerge];		oCounter++;
			counts[k] -= merged;
			numTotalPoints -= merged;

			/* Move to the next subpath. */
			base += tempCounts[k];
			toMerge = base + 1;
			merged = 0;
		}

		/* Free temp buffers. */
		OVG_SAFE_FREE(context->os, tempPoints);
		OVG_SAFE_FREE(context->os, tempCounts);
	}

	isDashed = (context->strokeDashPattern.size > 0);
    if (isDashed)        /* Dash enabled, split the path into dashed line tContext->segments. */
    {
        gctINT32        *numDashSegs;
        gctINT32        *numDashPoints;
        gctINT32        *numPointsInDashSegs;
        gctINT32        numTotalDashPoints;
        _VGTessPoint    *dashPoints;
        _VGTesstype     *pattern;
		_VGTesstype		patternLen = 0;
		_VGTesstype		dashLen = 0;

        dashPhaseNext = TESSTYPE(context->strokeDashPhase);
		OVG_MALLOC(context->os, memStart,
					sizeof(gctINT32*) * numSubs +
                    sizeof(_VGTessPoint*) * numSubs +
                    sizeof(gctINT32) * numSubs * 2 +
                    sizeof(_VGTesstype) * context->strokeDashPattern.size);

        dashedCounts    = (gctINT32**)memStart;
        dashedPoints    = (_VGTessPoint**)(dashedCounts + numSubs);
        numDashSegs     = (gctINT32*)(dashedPoints + numSubs);
        numDashPoints   = (gctINT32*)(numDashSegs + numSubs);
        pattern         = (_VGTesstype*)(numDashPoints + numSubs);

        for (i = 0; i < context->strokeDashPattern.size; ++i)
        {
			if (context->strokeDashPattern.items[i] > 0.0f)
				pattern[i] = context->strokeDashPattern.items[i];
			else
				pattern[i] = 0.0f;
			if ((i & 0x1) == 0)
			{
				patternLen += pattern[i];
			}
			dashLen += pattern[i];
        }

		if (patternLen < 0 || dashLen <= 0)
		{
			if (context->strokeCapStyle == VG_CAP_BUTT)
			{
				OVG_SAFE_FREE(context->os, memStart);
				gcmFOOTER_ARG("return=%d", 0);
				return 0;
			}
			else
			{
				isDashed = gcvFALSE;
			}
		}
		else
		{

			tempPoint = points;
			for (i = 0; i < numSubs; ++i)
			{
				numDashPoints[i] =  _DivideIntoDashLines(context, tempPoint,
														 counts[i],
														 pattern,
														 subPathClosed[i],
														 context->strokeDashPattern.size,
														 dashPhaseNext,
														 &dashPhaseNext,
														 &numDashSegs[i],
														 &dashedPoints[i],
														 &dashedCounts[i]);
				if (context->strokeDashPhaseReset)
					dashPhaseNext = TESSTYPE(context->strokeDashPhase);

				tempPoint += counts[i];
			}

			j = numSubs;
			numSubs = 0;
			numTotalDashPoints = 0;
			for (i = 0; i < j; ++i)
			{
				numSubs += numDashSegs[i];

				for (k = 0; k < numDashSegs[i]; ++k)
					numTotalDashPoints += dashedCounts[i][k];
			}

			if (numTotalDashPoints == 0)
			{
				OVG_SAFE_FREE(context->os, memStart);
				gcmFOOTER_ARG("return=%d", 0);
				return 0;
			}

			OVG_MALLOC(context->os, dashPoints, sizeof(_VGTessPoint) * numTotalDashPoints);
			OVG_MALLOC(context->os, numPointsInDashSegs, sizeof(gctINT32) * numSubs);

			tempPoint = dashPoints;
			l = 0;
			for (i = 0; i < j; ++i)
			{
				tempPoint2 = dashedPoints[i];
				for (k = 0; k < numDashSegs[i]; ++k)
				{
					numPointsInDashSegs[l++] = dashedCounts[i][k];
					OVG_MEMCOPY(tempPoint, tempPoint2, dashedCounts[i][k] * sizeof(_VGTessPoint));
					tempPoint += dashedCounts[i][k];
					tempPoint2 += dashedCounts[i][k];
				}

				OVG_SAFE_FREE(context->os, dashedPoints[i]);
				OVG_SAFE_FREE(context->os, dashedCounts[i]);
			}
			OVG_SAFE_FREE(context->os, memStart);

			counts = numPointsInDashSegs;
			points = dashPoints;
			numTotalPoints = numTotalDashPoints;
		}
    }

    if (numTotalPoints < 1)
    {
		gcmFOOTER_ARG("return=%d", 0);
        return 0;
    }

	/* Estimate the stream buffer size and allocate the buffer. */
	{
		float finalWidth = lineWidth * scale;

		/* Estimate the body points of the segments. */
		streamPipe.numStreamPts = (numTotalPoints - numSubs) * 6;
		streamPipe.numIndices = (numTotalPoints - numSubs) * 12;

		/* Estimate join points, at most, there are... */
		if (context->strokeJoinStyle == VG_JOIN_ROUND)
		{
			streamPipe.numStreamPts += (int)((numTotalPoints - numSubs) * gcmMAX(1.0f, finalWidth) * 3.14f);
			streamPipe.numIndices += (int)((numTotalPoints - numSubs) * (gcmMAX(1.0f, finalWidth) * 3.14f ) * 3);
		}
		else
		{
			streamPipe.numStreamPts += (numTotalPoints - numSubs)* 4;
			streamPipe.numIndices += (numTotalPoints - numSubs) * 6;
		}

		/* Estimate the cap points. */
		if (context->strokeCapStyle == VG_CAP_ROUND)
		{
			streamPipe.numStreamPts += (int)((float)numSubs * 2.0f * gcmMAX(1.0f, finalWidth) * 1.57f);
			streamPipe.numIndices += (int)((float)numSubs * 2.0f * (gcmMAX(1.0f, finalWidth) * 1.57f - 1) * 3);
		}
		else
		{
			streamPipe.numStreamPts += numSubs * 2 * 5;
			streamPipe.numIndices += numSubs * 2 * 9;
		}

		/* Allocate the buffer. */
		OVG_MALLOC(context->os, streamPipe.stream, sizeof(_VGVector2) * streamPipe.numStreamPts);
		OVG_MALLOC(context->os, streamPipe.indices, sizeof(_VGuint16) * streamPipe.numIndices);

		streamPipe.currIndex = streamPipe.currStreamPts = 0;
	}

	tempPoint = points;


BEGIN_PROFILE("ConstructStroke")
#if SPECIAL_1PX_STROKE
	if (TESS_ABS(context->tessContext.strokeWidth * context->tessContext.strokeScale - 1.0f) < 0.1f)
	{
		int vertCount = 0;
		unsigned short *indStart;
		unsigned short loopStart;
		/*Determine the count of points.*/
		if (streamPipe.numStreamPts < numTotalPoints)
		{
			_ExpandPipe(context, &streamPipe, numTotalPoints, (numTotalPoints - 1) * 2);
		}

		for (i = 0; i < numTotalPoints; i++)
		{
			streamPipe.stream[i] = points[i].coord;
		}

		streamPipe.currIndex = 0;
		indStart = streamPipe.indices;
		for (i = 0; i < numSubs; ++i)
		{
			if (!isDashed && subPathClosed[i])
				loopStart = vertCount;

			*indStart = vertCount;
			streamPipe.currIndex++;
			*(indStart + 1) = *indStart + 1;
			streamPipe.currIndex++;
			indStart += 2;
			for (j = 2; j < counts[i]; j++)
			{
				*indStart = *(indStart - 1);
				streamPipe.currIndex++;
				*(indStart + 1) = *indStart + 1;
				streamPipe.currIndex++;
				indStart += 2;
			}

			if (!isDashed && subPathClosed[i])
			{
				*indStart = *(indStart - 1);
				streamPipe.currIndex++;
				*(indStart + 1) = loopStart;
				streamPipe.currIndex++;
				indStart += 2;
			}
			vertCount += counts[i];
		}
		streamPipe.numStreamPts = numTotalPoints;
		streamPipe.numIndices = streamPipe.currIndex;
		gco3D_SetAntiAliasLine(context->engine, gcvTRUE);
		gco3D_SetAALineWidth(context->engine, 0.5f);
	}
	else
#endif
	if (isDashed)
	{
		numTotalPoints = 0;
        for (i = 0; i < numSubs; ++i)
        {
			_ConstructStroke(context, tempPoint,
											counts[i],
											gcvFALSE,
											&streamPipe
											);
            tempPoint += counts[i];
		}
		streamPipe.numStreamPts = streamPipe.currStreamPts;
		streamPipe.numIndices = streamPipe.currIndex;
	}
	else
	{
		numTotalPoints = 0;
        for (i = 0; i < numSubs; ++i)
        {
			_ConstructStroke(context, tempPoint,
											counts[i],
											subPathClosed[i],
											&streamPipe
											);
            tempPoint += counts[i];
		}
		streamPipe.numStreamPts = streamPipe.currStreamPts;
		streamPipe.numIndices = streamPipe.currIndex;
	}
END_PROFILE

	if (streamPipe.numStreamPts < 3)
	{
		OVG_SAFE_FREE(context->os, streamPipe.indices);
		OVG_SAFE_FREE(context->os, streamPipe.stream);
		gcmFOOTER_ARG("return=%d", 0);
		return 0;
	}

    path->tessellateResult.strokeBuffer.normalized = gcvFALSE;
    path->tessellateResult.strokeBuffer.size       = 2;
    path->tessellateResult.strokeBuffer.stride     = sizeof(_VGVector2);
    path->tessellateResult.strokeBuffer.elementCount = streamPipe.numStreamPts;
    path->tessellateResult.strokeBuffer.type       = VERTEX_TYPE;
    path->tessellateResult.strokeDirty             = gcvFALSE;



	path->tessellateResult.strokeIndexBuffer.count = streamPipe.numIndices;
	path->tessellateResult.strokeIndexBuffer.indexType = gcvINDEX_16;

#if SPECIAL_1PX_STROKE
	if (TESS_ABS(context->tessContext.strokeWidth * context->tessContext.strokeScale - 1.0f) < 0.1f)
	{
		path->tessellateResult.strokeDesc.primitiveCount = streamPipe.numIndices / 2;
		path->tessellateResult.strokeDesc.primitiveType = gcvPRIMITIVE_LINE_LIST;
	}
	else
	{
		path->tessellateResult.strokeDesc.primitiveCount = streamPipe.numIndices / 3;
		path->tessellateResult.strokeDesc.primitiveType = gcvPRIMITIVE_TRIANGLE_LIST;
	}
#else
	path->tessellateResult.strokeDesc.primitiveCount = streamPipe.numIndices / 3;
#endif
	/* Vertex stream. */
	gcmASSERT(path->tessellateResult.strokeBuffer.data.items == gcvNULL);
	path->tessellateResult.strokeBuffer.data.items = (_VGubyte*)streamPipe.stream;
	path->tessellateResult.strokeBuffer.data.size = path->tessellateResult.strokeBuffer.data.allocated = streamPipe.numStreamPts * sizeof(_VGVector2);

	/* Index stream. */
	gcmASSERT(path->tessellateResult.strokeIndexBuffer.data.items == gcvNULL);
	path->tessellateResult.strokeIndexBuffer.data.items = (_VGubyte*)streamPipe.indices;
	path->tessellateResult.strokeIndexBuffer.data.size = path->tessellateResult.strokeIndexBuffer.data.allocated = streamPipe.numIndices * sizeof(_VGuint16);

	numTotalPoints = streamPipe.numStreamPts;

    if (isDashed)
    {
        OVG_FREE(context->os, points);
        OVG_FREE(context->os, counts);
    }

	gcmFOOTER_ARG("return=%d", numTotalPoints);
    return numTotalPoints;
}

int	_DistMerge(_VGVector2 *p0, _VGVector2 *p1, _VGVector2 *p2, float limit)
{
	float dist;
	_VGVector2 vec[3];

	vec[0].x = p1->x - p0->x;
	vec[0].y = p1->y - p0->y;
	vec[1].x = p2->x - p1->x;
	vec[1].y = p2->y - p1->y;
	vec[2].x = p2->x - p0->x;
	vec[2].y = p2->y - p0->y;

	if (vec[1].x * vec[2].x + vec[1].y * vec[2].y < 0)
	{
		return 0;
	}

	dist = (vec[0].x * vec[1].y - vec[0].y * vec[1].x);
	dist *= dist;
	limit = limit * limit * 0.25f * (vec[2].x * vec[2].x + vec[2].y * vec[2].y);
	if (dist < limit)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


gctBOOL Tessellate(
    _VGContext  *context,
    _VGPath     *path,
	_VGMatrix3x3 *matrix
    )
{

    _VGVector2	    *points;
    gctINT32        *numPointsInSubPaths;
    gctINT32        numSubPaths;
    gctINT32        numPoints;
    gctINT32        i, j, k;
    _VGint16        *triangleIndices;
    gctINT32        triangleIndicesArrayLength;
    gctINT32        triangleCount;
    _VGVector2	    *copyToPoint, *copyFromPoint;
	gctINT32		offsetIndex = 0, offset = 0;

	gcmHEADER_ARG("context=0x%x path=0x%x matrix=0x%x", context, path, matrix);

    gcmASSERT(path->tessellateResult.flattenedFillPath.points != gcvNULL);

    numPoints = path->tessellateResult.flattenedFillPath.numPoints;
	/* Too few points to make a triangle. */
    if (numPoints < 3)
    {
		gcmFOOTER_ARG("return=%s", "FALSE");
        return gcvFALSE;
    }

	OVG_MALLOC(context->os, points, sizeof(_VGVector2) * numPoints);
	OVG_MALLOC(context->os, numPointsInSubPaths, sizeof(gctINT32) * path->tessellateResult.flattenedFillPath.numSubPaths);

    /* Remove the degenerated lines. Triangulation can not directly handle this, so it is necessary. */
	/* Get rid of the subpaths that have less than 3 points. */
	numPoints = 0;
	for (k = 0; k < path->tessellateResult.flattenedFillPath.numSubPaths; ++k)
	{
		if (path->tessellateResult.flattenedFillPath.numPointsInSubPath[k] >= 3)
		{
			for (i = 0; i < path->tessellateResult.flattenedFillPath.numPointsInSubPath[k]; i++)
			{
				(points + numPoints)[i] = (path->tessellateResult.flattenedFillPath.points + offset)[i].coord;
			}
			numPointsInSubPaths[offsetIndex] = path->tessellateResult.flattenedFillPath.numPointsInSubPath[k];
			offsetIndex++;
			numPoints += path->tessellateResult.flattenedFillPath.numPointsInSubPath[k];
		}
		offset += path->tessellateResult.flattenedFillPath.numPointsInSubPath[k];
	}
	numSubPaths = offsetIndex;

	if (numPoints <= 0)
	{
		OVG_FREE(context->os, points);
		OVG_FREE(context->os, numPointsInSubPaths);
		gcmFOOTER_ARG("return=%s", "FALSE");
		return gcvFALSE;
	}

	/* Optimize point count. */
	{
		gctINT32 base = 0, toMerge = 1;
		gctINT32 tabStop = 0;
		_VGTesstype scale = 0, limit = 0, limit2 = 0;
		float	xA, yA;

		scale = context->tessContext.strokeScale;
		if (scale == 0)
		{
			gcmFOOTER_ARG("return=%s", "FALSE");
			return gcvFALSE;
		}

		i = 0;
		limit = 0.5f / scale;
		xA = TESS_SQRT(matrix->m[0][0] * matrix->m[0][0] + matrix->m[1][0] * matrix->m[1][0]);
		yA = TESS_SQRT(matrix->m[0][1] * matrix->m[0][1] + matrix->m[1][1] * matrix->m[1][1]);
		limit2 = limit * (xA > yA ? yA / xA : xA / yA);
		for (k = 0; k < numSubPaths; ++k)
		{
			points[i] = points[tabStop];
			tabStop += numPointsInSubPaths[k];
			if (numPointsInSubPaths[k] < 4)		/* Such a subpath will be entirely copied. */
			{
				while(toMerge < tabStop)
				{
					points[++i] = points[toMerge++];
				}
				base += numPointsInSubPaths[k];
				++toMerge;
				++i;
				continue;
			}
			while (toMerge < tabStop)
			{
				if (tabStop - toMerge + i - base <= 2)		/* If only <=3 points left in this sub path, stop merging and just copy */
				{
					while(toMerge < tabStop)
					{
						points[++i] = points[toMerge++];
					}
					break;
				}

				if (TESS_DIAG(points[i].x - points[toMerge].x, points[i].y - points[toMerge].y) < limit ||
					(toMerge + 1 < tabStop &&
					 _DistMerge(&points[i], &points[toMerge], &points[toMerge + 1], limit2) == 1))
				{
					toMerge++;
				}
				else
				{
					i++;
					if (i < toMerge)
					{
						points[i] = points[toMerge];
					}
					toMerge++;
				}
			}
			i++;
			toMerge++;
			numPoints -= numPointsInSubPaths[k] - (i - base);
			numPointsInSubPaths[k] = i - base;
			base += numPointsInSubPaths[k];
		}
	}

    copyFromPoint = points + numPointsInSubPaths[0];   /* head of the second path point is the location for check start. */
    /* Get rid of the overlapped 2 points of "head-tail". */
    if (VECTOR_EQUAL(points[0], points[numPointsInSubPaths[0] - 1]))
    {
        --numPointsInSubPaths[0];
        --numPoints;
    }
    copyToPoint = points + numPointsInSubPaths[0];
    /* For a sub-path, if the first point and the last point lie on the same position, remove the last point.
       This is necessary for Triangulation. Why not get rid of the point in the flattening stage? Because
       the Stroke stage has different requirements from Triangulation, which requires the original point data
       be kept. */
    for (i = 1; i < numSubPaths; ++i)
    {
        if (copyToPoint < copyFromPoint)    /* Shrink the existing array. */
        {
            for (j = 0; j < numPointsInSubPaths[i]; ++j)
            {
                *copyToPoint++ = *copyFromPoint++;
            }
        }
        else
        {
            copyFromPoint += numPointsInSubPaths[i];
            copyToPoint += numPointsInSubPaths[i];
        }

        if (VECTOR_EQUAL((*(copyToPoint - 1)), (*(copyToPoint - numPointsInSubPaths[i]))))
        {
            --numPointsInSubPaths[i];
            --copyToPoint;
            --numPoints;
        }
    }

	/* Too few points to make a triangle. */
    if (numPoints < 3)      /* No points to process. */
    {

        OVG_SAFE_FREE(context->os, points);
        OVG_SAFE_FREE(context->os, numPointsInSubPaths);

		gcmFOOTER_ARG("return=%s", "FALSE");
		return gcvFALSE;
    }

BEGIN_PROFILE("Triangulation")
	triangleIndices = _Triangulation(context, &points,
                                     &numPoints,
                                     numPointsInSubPaths,
                                     numSubPaths,
                                     context->fillRule == VG_NON_ZERO,
                                     &triangleCount,
                                     &triangleIndicesArrayLength);
END_PROFILE

    OVG_SAFE_FREE(context->os, numPointsInSubPaths);
    if (triangleCount < 1 ||
		triangleIndices == gcvNULL)  /* No triangles to draw. */
    {
        OVG_SAFE_FREE(context->os, triangleIndices);
        OVG_SAFE_FREE(context->os, points);

		gcmFOOTER_ARG("return=%s", "FALSE");
		return gcvFALSE;
    }


	if (triangleCount < 10)
	{
		_VGVector2 *tempPoints;
		path->tessellateResult.fillDesc.primitiveCount = -triangleCount;
		OVG_MALLOC(context->os, tempPoints, sizeof(_VGVector2) * triangleCount * 3);
		for (i = 0; i < triangleCount; i++)
		{
			tempPoints[i * 3] = points[triangleIndices[i*3]];
			tempPoints[i * 3 + 1] = points[triangleIndices[i*3 + 1]];
			tempPoints[i * 3 + 2] = points[triangleIndices[i*3 + 2]];
		}
		OVG_SAFE_FREE(context->os, points);
		OVG_SAFE_FREE(context->os, triangleIndices);
		path->tessellateResult.vertexBuffer.data.items = (_VGubyte*)tempPoints;
		path->tessellateResult.vertexBuffer.data.size = path->tessellateResult.vertexBuffer.data.allocated = triangleCount * 3 * sizeof(_VGVector2);
	}
	else
	{
		path->tessellateResult.fillDesc.primitiveCount = triangleCount;
		/* Index buffer */
		path->tessellateResult.indexBuffer.indexType = gcvINDEX_16;
		path->tessellateResult.indexBuffer.count = triangleCount * 3;

		path->tessellateResult.indexBuffer.data.items = (_VGubyte*)triangleIndices;
		path->tessellateResult.indexBuffer.data.size = path->tessellateResult.indexBuffer.data.allocated = sizeof(_VGuint16) * triangleCount * 3;

		path->tessellateResult.vertexBuffer.data.items = (_VGubyte*)points;
		path->tessellateResult.vertexBuffer.data.size = path->tessellateResult.vertexBuffer.data.allocated = numPoints * sizeof(_VGVector2);
	}

    /* Fixed data.    */
    path->tessellateResult.vertexBuffer.elementCount   = 1;     /* One path is a single primitive. */
    path->tessellateResult.vertexBuffer.normalized = gcvFALSE;
    path->tessellateResult.vertexBuffer.size       = 2;
    path->tessellateResult.vertexBuffer.stride     = sizeof(_VGVector2);
    path->tessellateResult.vertexBuffer.type       = VERTEX_TYPE;
    path->tessellateResult.fillDirty               = gcvFALSE;

	gcmFOOTER_ARG("return=%s", "TRUE");
    return gcvTRUE;
}


/* Check whether it is an extreme bezier. See Frank's doc of flattening. */
gctBOOL    _IsExtremeBezier(
	_VGTessellationContext *tContext,
    _VGVector2  *cp
    )
{
    _VGTesstype sn, csn;
    _VGTesstype dx1, dy1, dx2, dy2, dx3, dy3;
    _VGTesstype dyy1, dyy2, dyy3, dxx1, dxx2;
    _VGTesstype midx, midy, midxx;
    _VGTesstype midx1, midy1, threshold;

    dx1 = cp[1].x - cp[0].x;
    dy1 = cp[1].y - cp[0].y;
    dx2 = cp[2].x - cp[0].x;
    dy2 = cp[2].y - cp[0].y;
    dx3 = cp[3].x - cp[0].x;
    dy3 = cp[3].y - cp[0].y;
    midx = ((cp[0].x + cp[2].x) * 0.25f) + cp[1].x * 0.5f;
    midy = ((cp[0].y + cp[2].y) * 0.25f) + cp[1].y * 0.5f;
    midx1 = ((cp[1].x + cp[3].x) * 0.25f) + cp[2].x * 0.5f;
    midy1 = ((cp[1].y + cp[3].y) * 0.25f) + cp[2].y * 0.5f;
    midx = (midx + midx1) * 0.5f;
    midy = (midy + midy1) * 0.5f;
    midx -= cp[0].x;
    midy -= cp[0].y;

    if (dx3 == 0)
    {
        sn = 0;
        csn = TESS_CONST_ONE;
    }else
    if (dy3 == 0)
    {
        sn = TESS_CONST_ONE;
        csn = 0;
    }
    else
    {
        sn = dx3;
        csn = dy3;
    }

    /* Transform the inner two control points. */
    dyy1 = dx1 * sn + dy1 * csn;
    dyy2 = dx2 * sn + dy2 * csn;
    dyy3 = dx3 * sn + dy3 * csn;
	dxx1 = dx1 * csn - dy1 * sn;
	dxx2 = dx2 * csn - dy2 * sn;
    midxx = midx * csn - midy * sn;
	threshold = tContext->eBezierThold * (sn*sn + csn*csn);

    /* Judge the position */
    if (((dxx1 * dxx2 < 0) ||
		 (((dyy1 < 0) && (dyy2 > dyy3)) ||
		  ((dyy1 > 0) && (dyy2 < dyy1)) ||
		  ((dyy1 ==0) && ((dyy2 < 0) || (dyy2 > dyy3))) ||
          (dyy3 == 0))) &&
		(midxx*midxx < threshold))
    {
        return gcvTRUE;
    }
    else
    {
        return gcvFALSE;
    }
}

gctBOOL	_IsCuspBezier(
	_VGVector2	cp[4]
)
{
	gctBOOL result = gcvFALSE;
	_VGVector2	vec[3];
	gctFLOAT	len[3];
	gctFLOAT	dp[2];

	/* Any co-exist points ? */
	if (VECTOR_EQUAL(cp[0], cp[1]) ||
		VECTOR_EQUAL(cp[1], cp[2]) ||
		VECTOR_EQUAL(cp[2], cp[3]) )
	{
		return gcvFALSE;
	}

	/* Check cusp angle and length. */
	vec[0].x = cp[1].x - cp[0].x;
	vec[0].y = cp[1].y - cp[0].y;
	vec[1].x = cp[2].x - cp[1].x;
	vec[1].y = cp[2].y - cp[1].y;
	vec[2].x = cp[3].x - cp[2].x;
	vec[2].y = cp[3].y - cp[2].y;

	len[0] = _Diag(vec[0].x, vec[0].y);
	len[1] = _Diag(vec[1].x, vec[1].y);
	len[2] = _Diag(vec[2].x, vec[2].y);

	dp[0] = vec[0].x * vec[1].x + vec[0].y * vec[1].y;
	dp[1] = vec[1].x * vec[2].x + vec[1].y * vec[2].y;

	if ( ((dp[0] <= 0.0f) && ((len[0] / len[1] >= 100.0f) || (len[1] / len[2] >= 100.0f))) ||
		 ((dp[1] <= 0.0f) && ((len[1] / len[2] >= 100.0f) || (len[2] / len[1] >= 100.0f))) )
	{
		result = gcvTRUE;
	}

	return result;
}

gctINT32 _BezierFlatten(
	_VGContext	*context,
	_VGVector2		cp[4],
    _VGTessPoint    **points
    )
{
    _VGTesstype    tempFloat;
	_VGVector2	tempVec[2];
	_VGTessellationContext	*tContext = &context->tessContext;
	_VGTesstype		t0, t1;
	gctFLOAT	eps_org = 0.0f, eps_org2 = 0.0f;
#if USE_TA
	gcoOS	os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x cp=0x%x points=0x%x",
				   context, cp, points);

    _ResetFlattenStates(tContext);

	/* Test whether it's the "sharp" case in conformance. */
	if (context->hardware.isConformance &&
		_IsCuspBezier(cp) &&
		context->tessContext.flattenForStroke)
	{
		/* Try a smaller epsilon for a very sharp bezier. */
		eps_org = context->tessContext.epsilon;
		eps_org2 = context->tessContext.epsilonSquare;
		context->tessContext.epsilon /= 100.0f;
		context->tessContext.epsilonSquare /= 10000.0f;
	}

    /* Calculate the tangent of the first point.*/
	if (tContext->flattenForStroke)
	{
		tempVec[1] = tempVec[0] = cp[0];
		if ( !VECTOR_EQUAL(cp[0], cp[1]))
			tempVec[1] = cp[1];
		else if ( !VECTOR_EQUAL(cp[0], cp[2]))
			tempVec[1] = cp[2];
		else
			tempVec[1] = cp[3];

		tempFloat = (_VGTesstype)TESS_DIAG(tempVec[1].x - tempVec[0].x, tempVec[1].y - tempVec[0].y);
		gcmASSERT(tempFloat > 0);
		tContext->segments[0].outTan.x = tContext->segments[0].inTan.x = TESS_DIV((tempVec[1].x - tempVec[0].x), tempFloat);
		tContext->segments[0].outTan.y = tContext->segments[0].inTan.y = TESS_DIV((tempVec[1].y - tempVec[0].y), tempFloat);
		tContext->segments[0].flags = POINT_SMOOTH_OUT;
	}
	else
		tContext->segments[0].flags = 0;
    tContext->segments[0].coord = cp[0];
    tContext->segCount = 1;

	if (context->hardware.isConformance)
	{
		t0 = (_VGTesstype)0;
		t1 = (_VGTesstype)1;
	}
	else
	{
		t0 = (_VGTesstype)-1000;
		t1 = (_VGTesstype)-1000;
	}

    if (_IsExtremeBezier(tContext, cp))
        _DivideBezier(tContext, cp, 1, 2, 2, t0, t1);
    else
	{
		if (_StepsNeeded(tContext, cp) > 0)
			_DivideBezier(tContext, cp, 0, 0, 1, t0, t1);
	}

    /* Calculate the tangent of the last point.*/
	if (tContext->flattenForStroke)
	{
		tempVec[1] = cp[3];
		if ( !VECTOR_EQUAL(cp[3], cp[2]))
			tempVec[0] = cp[2];
		else if ( !VECTOR_EQUAL(cp[3], cp[1]))
			tempVec[0] = cp[1];
		else
			tempVec[0] = cp[0];

		tempFloat = (_VGTesstype)TESS_DIAG(tempVec[1].x - tempVec[0].x, tempVec[1].y - tempVec[0].y);
		gcmASSERT (tempFloat > 0);
		tContext->segments[tContext->segCount].outTan.x = tContext->segments[tContext->segCount].inTan.x = TESS_DIV((tempVec[1].x - tempVec[0].x), tempFloat);
		tContext->segments[tContext->segCount].outTan.y = tContext->segments[tContext->segCount].inTan.y = TESS_DIV((tempVec[1].y - tempVec[0].y), tempFloat);
		tContext->segments[tContext->segCount].flags = POINT_SMOOTH_IN;
	}
	else
		tContext->segments[tContext->segCount].flags = 0;
    tContext->segments[tContext->segCount++].coord = cp[3];

#if USE_FTA
	TA_Resize(os, (void**)points, sizeof(_VGTessPoint) * tContext->segCount, 1);
#else
	OVG_MALLOC(context->os, *points, sizeof(_VGTessPoint) * tContext->segCount);
#endif
    OVG_MEMCOPY(*points, tContext->segments, sizeof(_VGTessPoint) * tContext->segCount);

	if (eps_org != 0.0f)
	{
		context->tessContext.epsilon = eps_org;
		context->tessContext.epsilonSquare = eps_org2;
	}

	gcmFOOTER_ARG("return=%d", tContext->segCount);
    return tContext->segCount;
}

static int added[2];
void _DivideBezier(
	_VGTessellationContext	*tContext,
    _VGVector2		p[],
    gctINT32        step,
    gctINT32        togo,
    gctINT32        checks,
	_VGTesstype		t0,
	_VGTesstype		t1
    )
{
    _VGVector2	p1[4];
    _VGVector2	p2[4];
    _VGVector2	pMid;
    gctINT32		togo1;
    gctINT32		togo2;
	_VGTesstype		tMid;

	if (t0 == 0 && t1 == 1)
	{
		added[0] = added[1] = 0;
	}

    p1[0] = p[0];
    p2[3] = p[3];
    MID_POINT(p[0],  p[1],  p1[1]);
    MID_POINT(p[2],  p[3],  p2[2]);
    MID_POINT(p[1],  p[2],  pMid);
    MID_POINT(p1[1], pMid,  p1[2]);
    MID_POINT(p2[2], pMid,  p2[1]);
    MID_POINT(p1[2], p2[1], p1[3]);
    p2[0] = p1[3];
	tMid = TESS_MUL((t0 + t1), TESS_CONST_HALF);

    togo1 = 0;
    togo2 = 0;
    if (step < BEZIER_MOST_STEPS)
    {
		if (togo == 0 && checks > 0)	/* Need to check distance */
		{
			togo1 = _StepsNeeded(tContext, p1);
			togo2 = _StepsNeeded(tContext, p2);

            if (!tContext->flattenForStroke)
            {
			    --checks;
            }
		}
		else
		{
			togo1 = togo2 = togo;
		}
    }
    if (togo1 > 0)
    {
        _DivideBezier(tContext, p1, step + 1, togo1 - 1, checks, t0, tMid);
    }

	if (tContext->flattenForStroke)
	{
		tContext->segments[tContext->segCount].flags = POINT_SMOOTH;
	}
	else
		tContext->segments[tContext->segCount].flags = 0;

	if(t0 != -1000 && t1 != -1000)
	{ /* t = 	0.289957987 0.710042013 to the flatten  */
		int i;
		_VGTesstype flattenT = 0.289957987f;
		for(i = 0; i<2; i++){
			if(i)
				flattenT = 1.0f-flattenT;
			if(added[i])
				continue;
			if(tMid > flattenT && t0 < flattenT){
				_VGTesstype t = (flattenT - t0)/(tMid-t0);
				tContext->segments[tContext->segCount].coord.x
					= p1[0].x * (1.0f-t)*(1.0f-t)*(1.0f-t) +
					  3.0f*p1[1].x * (1.0f-t)*(1.0f-t)*t +
					  3.0f*p1[2].x * (1.0f-t)*t*t +
					  p1[3].x * t*t*t;
				tContext->segments[tContext->segCount].coord.y
				= p1[0].y * (1.0f-t)*(1.0f-t)*(1.0f-t) +
					  3.0f*p1[1].y * (1.0f-t)*(1.0f-t)*t +
					  3.0f*p1[2].y * (1.0f-t)*t*t +
					  p1[3].y * t*t*t;
				tContext->segCount++;
				if (tContext->flattenForStroke)
				{
					tContext->segments[tContext->segCount].flags = POINT_SMOOTH;
				}
				else
					tContext->segments[tContext->segCount].flags = 0;

				added[i] = 1;
				break;
			}
		}
	}

    tContext->segments[tContext->segCount++].coord = p2[0];
    if (togo2 > 0)
    {
        _DivideBezier(tContext, p2, step + 1, togo2 - 1, checks, tMid, t1);
    }
}

/* x and y are the two semi axes, and a is the tilte angle. P1 and P2 are two
 points on the ellipse, and majorArc is a flag indicating whether the
 ellipse arc is over or under 180 degrees.
 This function locate the center of the ellipse, as well as the angles
 (a1 and a2) representing the two points. */
gctINT32 _CreateEllipse(
    _VGTesstype     x,
    _VGTesstype     y,
    _VGTesstype     a,
    _VGVector2		*p1,
    _VGVector2		*p2,
    gctINT32        majorArc,
    gctBOOL         forcedSolution,
    _VGTesstype     *a1,
    _VGTesstype     *a2,
    _VGEllipse      *result
    )
{
	_VGVector2		center;
    _VGVector2		np1;
    _VGVector2		np2;
    _VGVector2		mid;
    _VGTesstype		vx;
    _VGTesstype		vy;
    _VGTesstype		k;
	_VGVector2		nCenter;	/* center of the unit circle */
    _VGTesstype		A,B;
    _VGTesstype		c, s, px, py;
	_VGTesstype		offsetX, offsetY;

	gcmHEADER_ARG("x=%f y=%f a=%f p1=0x%x p2=0x%x majorArc=%d forcedSolution=%s a1=0x%x a2=0x%x result=0x%x",
				   x, y, a, p1, p2, majorArc,
				   forcedSolution ? "TRUE" : "FALSE",
				   a1, a2, result);

    if ((p1->x == p2->x) && (p1->y == p2->y))
	{
		gcmFOOTER_ARG("return=%d", 0);
        return 0;
	}

	if (x * 100000 < y || y * 100000 < x)
	{
		MID_POINT((*p1), (*p2), (result->center));
		if (p1->y == p2->y)
		{
			if (p1->x < p2->x)
			{
				*a1 = gcdPI;
				*a2 = gcdPI * 2.0f;
			}
			else
			{
				*a1 = 0.0f;
				*a2 = gcdPI;
			}
		}
		else
		{
			if (p1->y < p2->y)
			{
				*a1 = gcdPI * 1.5f;
				*a2 = gcdPI * 2.5f;
			}
			else
			{
				*a1 = gcdPI * 0.5f;
				*a2 = gcdPI * 1.5f;
			}
		}

		result->angle = a;

		gcmFOOTER_ARG("return=%d", 1);
		return 1;
	}


	offsetX = offsetY = 0;
	p1->x -= offsetX;
	p2->x -= offsetX;
	p1->y -= offsetY;
	p2->y -= offsetY;

	c = TESS_COS(a);
	s = TESS_SIN(a);
	px = TESS_MUL(p1->x, c) + TESS_MUL(p1->y, s);	/* projection of P1 on major axis */
	py = TESS_MUL(-p1->x, s) + TESS_MUL(p1->y, c);	/* projection of P1 on minor axis */

    A = px;
    B = py;
    np1.x = TESS_DIV(TESS_MUL(px, c), x) - TESS_DIV(TESS_MUL(py, s), y);
    np1.y = TESS_DIV(TESS_MUL(px, s), x) + TESS_DIV(TESS_MUL(py, c), y);	            /* Normalized point when the ellipse is normalized to the unit circle */
	px =  TESS_MUL(p2->x, c) + TESS_MUL(p2->y, s);
	py = TESS_MUL(-p2->x, s) + TESS_MUL(p2->y, c);
    A -= px;
    B -= py;
    np2.x = TESS_DIV(TESS_MUL(px, c), x) - TESS_DIV(TESS_MUL(py, s), y);
    np2.y = TESS_DIV(TESS_MUL(px, s), x) + TESS_DIV(TESS_MUL(py, c), y);
    np1.x = (_VGTesstype)gcoMATH_Floor( np1.x / ZEROERROR + 0.5f) * ZEROERROR;
    np1.y = (_VGTesstype)gcoMATH_Floor( np1.y / ZEROERROR + 0.5f) * ZEROERROR;
    np2.x = (_VGTesstype)gcoMATH_Floor( np2.x / ZEROERROR + 0.5f) * ZEROERROR;
    np2.y = (_VGTesstype)gcoMATH_Floor( np2.y / ZEROERROR + 0.5f) * ZEROERROR;

    MID_POINT(np1, np2, mid);
	vx = np1.x - np2.x;
	vy = np1.y - np2.y;

    /* calculating the distance from mid to normalized center */
	k = TESS_MUL(vx, vx) + TESS_MUL(vy, vy);
	if (k == 0)
	{
		k = ZEROERROR;
	}
	k = TESS_DIV( TESS_CONST_ONE, k) - TESS_CONST_ONE_FOURTH;
	if (k < -ZEROERROR)		/* No such ellipse exists (distance between the two normalized points > 1) */
	{
        if (forcedSolution)
        {
            _VGTesstype    c1;
            _VGVector2  pp0, pp1;
			_VGTesstype temp;
            _VGTesstype len = TESS_MUL( TESS_DIAG(p1->x - p2->x, p1->y - p2->y) ,TESS_CONST_HALF);
            pp0.x = TESS_MUL((p1->x - p2->x) , TESS_CONST_HALF);
            pp0.y = TESS_MUL((p1->y - p2->y) , TESS_CONST_HALF);
            pp1.x = (TESS_MUL(c, pp0.x) - TESS_MUL(-s, pp0.y));
            pp1.y = (TESS_MUL(-s, pp0.x) + TESS_MUL(c, pp0.y));
            c1 = TESS_DIV(y, x);

			pp0.x = TESS_ABS(TESS_MUL(pp1.x,c1));
			pp0.y = TESS_ABS(pp1.y);
			if(pp0.x>=pp0.y){
				temp = TESS_DIV(pp0.y,pp0.x);
				result->vAxis = TESS_MUL(TESS_DIAG(TESS_CONST_ONE,temp),pp0.x);
			}
			else{
				temp = TESS_DIV(pp0.x,pp0.y);
				result->vAxis = TESS_MUL(TESS_DIAG(TESS_CONST_ONE,temp),pp0.y);
			}
			result->hAxis = TESS_DIV(result->vAxis,c1);

			MID_POINT((*p1), (*p2), result->center);
            *a1 = TESS_ACOS(TESS_DIV(pp1.x, len));

			if (pp1.y < 0)
            {
                *a1 = TESS_PI_DOUBLE - (*a1);
            }
            *a2 = *a1 + gcdPI;
            result->angle = a;

			result->center.x += offsetX;
			result->center.y += offsetY;
			p1->x += offsetX;
			p2->x += offsetX;
			p1->y += offsetY;
			p2->y += offsetY;

			gcmFOOTER_ARG("return=%d", 1);
			return 1;
        }
        else
        {
			*a1 = *a2 = 0.0f;
            result->hAxis = 0;
            result->vAxis = 0;
            result->angle = 0;
            result->center.x = result->center.y = 0;
			p1->x += offsetX;
			p2->x += offsetX;
			p1->y += offsetY;
			p2->y += offsetY;

			gcmFOOTER_ARG("return=%d", 0);
            return 0;
        }
	}

    if ( (k > -ZEROERROR) && (k < ZEROERROR) )
    {
        k = 0;
    }

	k = TESS_SQRT(k);
    if (majorArc == 0)
    {
        nCenter.x = mid.x + TESS_MUL(k, vy);
        nCenter.y = mid.y - TESS_MUL(k, vx);
    }
    else
    {
        nCenter.x = mid.x - TESS_MUL(k, vy);
        nCenter.y = mid.y + TESS_MUL(k, vx);
    }
	px = TESS_MUL(nCenter.x, c) + TESS_MUL(nCenter.y, s);
	py = TESS_MUL(-nCenter.x, s) + TESS_MUL(nCenter.y, c);
    center.x = TESS_MUL(px, TESS_MUL(c, x)) - TESS_MUL(py, TESS_MUL(s, y));
    center.y = TESS_MUL(px, TESS_MUL(s, x)) + TESS_MUL(py, TESS_MUL(c, y)); 	/* stretch out to get the real center */
	/* calculating the angles */
	px = np1.x - nCenter.x;
	py = np1.y - nCenter.y;
    if (px > TESS_CONST_ONE)
        px = TESS_CONST_ONE;
    else if (px < -TESS_CONST_ONE)
        px = -TESS_CONST_ONE;
	*a1 = TESS_ACOS(px);
	if (py < 0)
	{
		*a1 = -(*a1);
	}
	*a1 -= a;
	if (*a1 < 0)
	{
		*a1 += TESS_PI_DOUBLE;
	}
	px = np2.x - nCenter.x;
	py = np2.y - nCenter.y;
    if (px > TESS_CONST_ONE)
        px = TESS_CONST_ONE;
    else if (px < -TESS_CONST_ONE)
        px = -TESS_CONST_ONE;
	*a2 = TESS_ACOS(px);
	if (py < 0)
	{
		*a2 = -(*a2);
	}
	*a2 -= a;
	while (*a1 > *a2)
	{
		*a2 += TESS_PI_DOUBLE;
	}

	center.x += offsetX;
	center.y += offsetY;
	p1->x += offsetX;
	p2->x += offsetX;
	p1->y += offsetY;
	p2->y += offsetY;

    result->center = center;
    result->hAxis = x;
    result->vAxis = y;
    result->angle = a;

    if (*a1 == *a2)
	{
		gcmFOOTER_ARG("return=%d", 0);
        return 0;
	}

	gcmFOOTER_ARG("return=%d", 1);
    return 1;
}


/* Main work routine for ellipse flattening. It accepts a start and finish
 angle, and a start and finish point. The redundance is due to the
 calculation is done already when creating the ellipse, where it is easier.
 Over loaded functions can be created to repeat the calculation. It is
 assumed 0 <= start angle <= end angle and end angle - start angle <= 2pi */
gctINT32 _EllipseFlatten(
	_VGContext	*context,
    _VGEllipse	*aEllipse,
    _VGTesstype startAngle,
    _VGTesstype endAngle,
    _VGVector2	*startPoint,
    _VGVector2	*endPoint,
    _VGTessPoint **result,
	gctINT32 offset
    )
{
    gctINT32	i;
	gctINT32	startQuadrant, endQuadrant;
	gctINT32	startPos, endPos;	/* position of start and end points in the tContext->segments array */
    gctINT32	newSegCount;
    gctINT32	p;
    gctINT32	q;
	_VGTesstype normStartAngle, normEndAngle;
    _VGTesstype c;
    _VGTesstype s;
    _VGTesstype x;
    _VGTesstype y;
	_VGVector2	start, end, middle;
    _VGVector2	circlePoints[3];
    _VGVector2	tanVec;
    _VGTesstype     tempVal;
	_VGTessellationContext	*tContext = &context->tessContext;
#if USE_TA
	gcoOS	os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x aEllipse=0x%x startAngle=%f endAngle=%f startPoint=0x%x endPoint=0x%x result=0x%x offset=%d",
				  context, aEllipse, startAngle, endAngle, startPoint, endPoint, result, offset);

    /* Clear golbal variables. */
    _ResetFlattenStates(tContext);

	/* Amend the start and end angles according to the hAxis and vAxis. Because in CreateEllipse(), the angle is consistent with the hAxis and vAxis, that is,
	   the actual ellipse; but here, the angle is used as if the ellipse is a circle (hAxis equals to vAxis), we need to map the angles to a circle. */
	if (aEllipse->hAxis != aEllipse->vAxis && \
		aEllipse->hAxis * 100000 > aEllipse->vAxis && \
		aEllipse->vAxis * 100000 > aEllipse->hAxis
		)
	{
		gctINT32 steps;
		gctFLOAT dx0, dy0, dx1, dy1, cosVal;
		_VGVector2	np0, np1;

		dx0 = startPoint->x - aEllipse->center.x;
		dy0 = startPoint->y - aEllipse->center.y;
		dx1 = endPoint->x - aEllipse->center.x;
		dy1 = endPoint->y - aEllipse->center.y;
		np0.x = (gctFLOAT)gcoMATH_Cosine(aEllipse->angle) * dx0 + (gctFLOAT)gcoMATH_Sine(aEllipse->angle) * dy0;
		np1.x = (gctFLOAT)gcoMATH_Cosine(aEllipse->angle) * dx1 + (gctFLOAT)gcoMATH_Sine(aEllipse->angle) * dy1;

		steps = (gctINT32)(startAngle / gcdPI);
		cosVal = np0.x / aEllipse->hAxis;
		if (cosVal > 1.0f)
			cosVal = 1.0f;
		else
		if (cosVal < -1.0f)
			cosVal = -1.0f;
		cosVal = TESS_ACOS(cosVal);
		if (steps % 2 == 0)
		{
			if (startAngle >= 0)
				startAngle = steps * gcdPI + cosVal;
			else
				startAngle = steps * gcdPI - cosVal;
		}
		else
		{
			if (startAngle >= 0)
				startAngle = (steps + 1) * gcdPI - cosVal;
			else
				startAngle = (steps - 1) * gcdPI + cosVal;
		}

		steps = (gctINT32)(endAngle / gcdPI);
		cosVal = np1.x / aEllipse->hAxis;
		if (cosVal > 1.0f)
			cosVal = 1.0f;
		else
		if (cosVal < -1.0f)
			cosVal = -1.0f;
		cosVal = TESS_ACOS(cosVal);
		if (steps % 2 == 0)
		{
			if (endAngle >= 0)
				endAngle = steps * gcdPI + cosVal;
			else
				endAngle = steps * gcdPI - cosVal;
		}
		else
		{
			if (endAngle >= 0)
				endAngle = (steps + 1) * gcdPI - cosVal;
			else
				endAngle = (steps - 1) * gcdPI + cosVal;
		}
	}
    /* Number of tContext->iterations needed at the two extreme points (0,b) and (a,0) */

	x = _MockLog2(TESS_MUL(TESS_PI_OCT,
                           TESS_SQRT(TESS_DIV(TESS_MUL(TESS_CONST_2, aEllipse->vAxis),
                                               tContext->epsilon))));
	y = _MockLog2(TESS_MUL(TESS_PI_OCT,
                           TESS_SQRT(TESS_DIV(TESS_MUL(TESS_CONST_2, aEllipse->hAxis),
                                               tContext->epsilon))));
	if (aEllipse->hAxis >= aEllipse->vAxis)
	{
		tContext->direction = 1;
	}
	else	/* swap x and y */
	{
		_VGTesstype z = x;
		tContext->direction = -1;
		x = y;
		y = z;
	}
	/* Define number of tContext->iterations needed at each position
	 For positions between tContext->iterations[i-1] and tContext->iterations[i], i tContext->iterations needed */
	for (i = 0; i < 8; ++i)
	{
		if (TESSTYPE(i) > y)
		{
			if (TESSTYPE(i) > x)
			{
				tContext->iterations[i] = TESS_CONST_ONE;
			}
			else		/* Fit into a Cubic curve */
			{
				tContext->iterations[i] = _MockCubeRoot( TESS_DIV((TESSTYPE(i) - x), (y - x)));
			}
		}
	}

	/* Find start and end pos and type of arc */
	normStartAngle = TESS_MUL(TESS_MUL(startAngle, TESS_CONST_2), TESS_PI_RCP);	/* Normalize to [0, 1] */
	normEndAngle = TESS_MUL(TESS_MUL(endAngle, TESS_CONST_2), TESS_PI_RCP);
	startQuadrant = TESS_INT(TESS_FLOOR(normStartAngle)) + 1;
	endQuadrant = TESS_INT(TESS_CEIL(normEndAngle));

	normStartAngle = (startQuadrant % 2 == 0) ? TESSTYPE(startQuadrant) - normStartAngle : normStartAngle - TESSTYPE(startQuadrant - 1);
	normEndAngle = (endQuadrant % 2 == 0) ? TESSTYPE(endQuadrant) - normEndAngle : normEndAngle - TESSTYPE(endQuadrant - 1);


	if (normStartAngle <= normEndAngle)
	{
		tContext->lowAngle = normStartAngle;
		tContext->highAngle = normEndAngle;
		tContext->arcInside[0] = (startQuadrant % 2 == 0) ? 1 : 0;	/* lower is inside */
		tContext->arcInside[1] = (endQuadrant % 2 == 0) ? 0 : 1;	/* upper is inside */
	}
	else
	{
		tContext->lowAngle = normEndAngle;
		tContext->highAngle = normStartAngle;
		tContext->arcInside[0] = (endQuadrant % 2 == 0) ? 0 : 1;	/* upper is inside */
		tContext->arcInside[1] = (startQuadrant % 2 == 0) ? 1 : 0;	/* lower is inside */
	}

	tContext->lowPos = (tContext->arcInside[0] == 0) ? -1 : 1000;
	tContext->highPos = (tContext->arcInside[1] == 0) ? -1 : 1000;

	if (endQuadrant == startQuadrant)
	{
		tContext->arcType = 0;	/* arc from start to end */
	}
	else if (endQuadrant - startQuadrant == 1)
	{
		if (startQuadrant % 2 == 0)
		{
			tContext->arcType = 1; /* arc from start down to 0 and back to end */
		}
		else
		{
			tContext->arcType = 2; /* arc from start to 1 and back to end */
		}
	}
	else
	{
		tContext->arcType = 3; /* arc from start to 1, back to 0, and to end */
	}

	/* Most number of points = 2^MaxNumberofIterations+1
     Get the 3 starting points */
	c = TESS_COS(aEllipse->angle);
	s = TESS_SIN(aEllipse->angle);
	if (aEllipse->angle == 0)
	{
		start.x = aEllipse->center.x + aEllipse->hAxis;
        start.y = aEllipse->center.y;
		end.x = aEllipse->center.x;
        end.y = aEllipse->center.y + aEllipse->vAxis;
		middle.x = aEllipse->center.x + aEllipse->hAxis;
        middle.y = aEllipse->center.y + aEllipse->vAxis;
	}
	else
	{   /*fixed unsafe*/
		start.x = aEllipse->center.x + TESS_MUL(c, aEllipse->hAxis);
        start.y = aEllipse->center.y + TESS_MUL(s, aEllipse->hAxis);
		end.x = aEllipse->center.x - TESS_MUL(s, aEllipse->vAxis);
        end.y = aEllipse->center.y + TESS_MUL(c, aEllipse->vAxis);
		middle.x = aEllipse->center.x + TESS_MUL(c, aEllipse->hAxis) - TESS_MUL(s, aEllipse->vAxis);
        middle.y = aEllipse->center.y + TESS_MUL(s, aEllipse->hAxis) + TESS_MUL(c, aEllipse->vAxis);
	}

	/* start working on the first quadrant */
	tContext->segCount = 0;
	tContext->segments[tContext->segCount].flags = 0;

    tContext->segments[tContext->segCount++].coord = start;
    circlePoints[0] = start;
    circlePoints[1] = middle;
    circlePoints[2] = end;
	_DivideCircle(tContext, circlePoints, 0, TESS_CONST_HALF, TESS_CONST_ONE_FOURTH);

	if (tContext->lowPos < 0)
	{
		tContext->lowPos = 0;
	}else
	if (tContext->lowPos > tContext->segCount)
	{
		tContext->lowPos = tContext->segCount;
	}
	if (tContext->highPos < 0)
	{
		tContext->highPos = 0;
	}else
	if (tContext->highPos > tContext->segCount)
	{
		tContext->highPos = tContext->segCount;
	}

	if (tContext->flattenForStroke)
	{
		/* Calculate the tangent vector. */
		tContext->segments[tContext->segCount].inTan.x = tContext->segments[tContext->segCount].outTan.x = -TESS_COS(aEllipse->angle);
		tContext->segments[tContext->segCount].inTan.y = tContext->segments[tContext->segCount].outTan.y = -TESS_SIN(aEllipse->angle);
		tContext->segments[tContext->segCount].flags = POINT_SMOOTH;
	}
	else
		tContext->segments[tContext->segCount].flags = 0;

    tContext->segments[tContext->segCount++].coord = end;

	if (normStartAngle < normEndAngle)
	{
		startPos = tContext->lowPos;
		endPos = tContext->highPos;
	}
	else
	{
		startPos = tContext->highPos;
		endPos = tContext->lowPos;
	}

    /* reflecting to the real quadrants */
#if USE_FTA
	TA_Resize(os, (void**)result, sizeof(_VGTessPoint) * tContext->segCount * 4 + offset, 1);
#else
	OVG_MALLOC(context->os, *result, sizeof(_VGTessPoint) * tContext->segCount * 4 + offset);
    OVG_MEMSET(*result, 0, sizeof(_VGTessPoint) * tContext->segCount * 4 + offset);
#endif
	*result += offset;
	newSegCount = 0;
    /* Calculate the tangent for this point. */
	if (tContext->flattenForStroke)
	{
		tanVec.x = TESS_MUL(-aEllipse->hAxis , TESS_SIN(startAngle));
		tanVec.y = TESS_MUL(aEllipse->vAxis , TESS_COS(startAngle));
		tempVal = (_VGTesstype)TESS_DIAG(tanVec.x, tanVec.y);
		tanVec.x = TESS_DIV(tanVec.x ,tempVal);
		tanVec.y = TESS_DIV(tanVec.y ,tempVal);
		(*result)[newSegCount].inTan.x = (*result)[newSegCount].outTan.x = TESS_MUL(TESS_COS(aEllipse->angle) , tanVec.x) - TESS_MUL(TESS_SIN(aEllipse->angle), tanVec.y);
		(*result)[newSegCount].inTan.y = (*result)[newSegCount].outTan.y = TESS_MUL(TESS_SIN(aEllipse->angle) , tanVec.x) + TESS_MUL(TESS_COS(aEllipse->angle), tanVec.y);
		(*result)[newSegCount].flags = POINT_SMOOTH_OUT;
	}
	else
		(*result)[newSegCount].flags = 0;

    (*result)[newSegCount++].coord = *startPoint;
	if (startQuadrant == endQuadrant)
	{
		if (startQuadrant % 2 == 0)
		{
			for (p = tContext->segCount - 2; p > 0; --p)
			{
                _ReflectPoint(&tContext->segments[p], startQuadrant, &aEllipse->center, c, s, &(*result)[newSegCount++]);
			}
		}
		else
		{
			for (p = 1; p < tContext->segCount-1; ++p)
			{
                _ReflectPoint(&tContext->segments[p], startQuadrant, &aEllipse->center, c, s, &(*result)[newSegCount++]);
			}
		}
	}
	else
	{
		if (startQuadrant % 2 == 0)
		{
			for (p = gcmMIN(tContext->segCount - 2, startPos - 1); p >= 0; --p)
			{
                _ReflectPoint(&tContext->segments[p], startQuadrant, &aEllipse->center, c, s, &(*result)[newSegCount++]);
			}
		}
		else
		{
			for (p = gcmMAX(1, startPos + 1); p < tContext->segCount; ++p)
			{
                _ReflectPoint(&tContext->segments[p], startQuadrant, &aEllipse->center, c, s, &(*result)[newSegCount++]);
			}
		}
		for (q = startQuadrant + 1; q < endQuadrant; ++q)
		{
			if (q % 2 == 0)
			{
				for (p = tContext->segCount - 2; p >= 0; --p)
				{
                    _ReflectPoint(&tContext->segments[p], q, &aEllipse->center, c, s, &(*result)[newSegCount++]);
				}
			}
			else
			{
				for (p = 1; p < tContext->segCount; ++p)
				{
                    _ReflectPoint(&tContext->segments[p], q, &aEllipse->center, c, s, &(*result)[newSegCount++]);
				}
			}
		}
		if (endQuadrant % 2 == 0)
		{
			for (p = tContext->segCount - 2; p > endPos; --p)
			{
                _ReflectPoint(&tContext->segments[p], endQuadrant, &aEllipse->center, c, s, &(*result)[newSegCount++]);
			}
		}
		else
		{
			endPos = gcmMIN(tContext->segCount - 1, endPos);
			for (p = 1; p < endPos; ++p)
			{
                _ReflectPoint(&tContext->segments[p], endQuadrant, &aEllipse->center, c, s, &(*result)[newSegCount++]);
			}
		}
	}

	if (tContext->flattenForStroke)
	{
		/* Calculate the tangent for this point. */
		tanVec.x = TESS_MUL(-aEllipse->hAxis, TESS_SIN(endAngle));
		tanVec.y = TESS_MUL( aEllipse->vAxis, TESS_COS(endAngle));
		tempVal = (_VGTesstype)TESS_DIAG(tanVec.x, tanVec.y);
		tanVec.x = TESS_DIV(tanVec.x, tempVal);
		tanVec.y = TESS_DIV(tanVec.y, tempVal);
		(*result)[newSegCount].inTan.x = (*result)[newSegCount].outTan.x = TESS_MUL(TESS_COS(aEllipse->angle) , tanVec.x) - TESS_MUL(TESS_SIN(aEllipse->angle) , tanVec.y);
		(*result)[newSegCount].inTan.y = (*result)[newSegCount].outTan.y = TESS_MUL(TESS_SIN(aEllipse->angle) , tanVec.x) + TESS_MUL(TESS_COS(aEllipse->angle) , tanVec.y);
		(*result)[newSegCount].flags = POINT_SMOOTH_IN;
	}
	else
		(*result)[newSegCount].flags = 0;
    (*result)[newSegCount++].coord = *endPoint;

	*result -= offset;

	gcmFOOTER_ARG("return=%d", newSegCount);
    return newSegCount;
}

/* Reflecting a point to other quadrants */
void _ReflectPoint(
    _VGTessPoint	*p,
    gctINT32		quadrant,
    _VGVector2		*center,
    _VGTesstype		c,
    _VGTesstype		s,
    _VGTessPoint	*result
    )
{
	_VGTesstype x, y;

	gcmHEADER_ARG("p=0x%x quadrant=%d center=0x%x c=%f s=%f result=0x%x",
		p, quadrant, center, c, s, result);

    switch (quadrant % 4)
	{
	case 1:
		*result = *p;
		break;
	case 2:
	    result->flags = p->flags;
		x = TESS_MUL((p->coord.x - center->x), c) + TESS_MUL((p->coord.y - center->y), s);
		y = TESS_MUL((p->coord.x - center->x), (-s)) + TESS_MUL((p->coord.y - center->y), c);
        result->coord.x = center->x - TESS_MUL(c, x) - TESS_MUL(s, y);
        result->coord.y = center->y - TESS_MUL(s, x) + TESS_MUL(c, y);
		break;
	case 3:
	    result->flags = p->flags;
		result->coord.x = TESS_MUL(TESS_CONST_2, center->x) - p->coord.x;
        result->coord.y = TESS_MUL(TESS_CONST_2, center->y) - p->coord.y;
		break;
	default:
	    result->flags = p->flags;
		x = TESS_MUL((p->coord.x - center->x), c) + TESS_MUL((p->coord.y - center->y), s);
		y = TESS_MUL((p->coord.x - center->x), (-s)) + TESS_MUL((p->coord.y - center->y), c);
		result->coord.x = center->x + TESS_MUL(c, x) + TESS_MUL(s, y);
        result->coord.y = center->y + TESS_MUL(s, x) - TESS_MUL(c, y);
		break;
	}

	gcmFOOTER_NO();
}


/* Iterations for ellipse flattening. P contains the three points for the
 rational Quadratic Bezier representation of the current ellipse arc
 (two points on the ellipse and the intersection of the tangent lines).
 i is a counter of tContext->iterations so that we know how to split the arc and when
 to stop. pos is the angle of the current center point, and inc indicating
 how far the next positions are (pos+inc and pos-inc). */
void _DivideCircle(
	_VGTessellationContext	*tContext,
    _VGVector2		p[],
    gctINT32		i,
    _VGTesstype		pos,
    _VGTesstype		inc
    )
{
	_VGVector2		p1[3];
	_VGVector2		p2[3];
    gctBOOL			needToGo;

	/* Split arc */
	p1[0] = p[0];
	p2[2] = p[2];
	p1[1].x = TESS_DIV((p[0].x + TESS_MUL(cosine[i], p[1].x)), (TESS_CONST_ONE + cosine[i]));
    p1[1].y = TESS_DIV((p[0].y + TESS_MUL(cosine[i], p[1].y)), (TESS_CONST_ONE + cosine[i]));
	p2[1].x = TESS_DIV((p[2].x + TESS_MUL(cosine[i], p[1].x)), (TESS_CONST_ONE + cosine[i]));
    p2[1].y = TESS_DIV((p[2].y + TESS_MUL(cosine[i], p[1].y)), (TESS_CONST_ONE + cosine[i]));
    MID_POINT(p1[1], p2[1], p1[2]);
	p2[0] = p1[2];

	/* Check whether need to go down further to the front half */
	if ((tContext->arcType & 1) == 1 ||
		pos >= tContext->lowAngle)
	{
		if ((tContext->direction == 1 && pos > tContext->iterations[i+1]) ||
            (tContext->direction == -1 && TESS_CONST_ONE - pos + TESS_MUL(TESS_CONST_2, inc) > tContext->iterations[i+1]))
		{
			_DivideCircle(tContext, p1, i + 1, pos - inc, TESS_MUL(inc, TESS_CONST_HALF));
		}
	}

	/* check whether point is on the arc */
	needToGo = gcvTRUE;
	switch (tContext->arcType)
	{
		case 0:
			if (pos < tContext->lowAngle || pos > tContext->highAngle)
			{
				needToGo = gcvFALSE;
			}
			break;
		case 1:
			if (pos > tContext->highAngle)
			{
				needToGo = gcvFALSE;
			}
			break;
		case 2:
			if (pos < tContext->lowAngle)
			{
				needToGo = gcvFALSE;
			}
			break;
		case 3:
			break;
	}

	if ((tContext->arcInside[0] == 0 && pos <= tContext->lowAngle && tContext->lowPos < tContext->segCount) ||
		(tContext->arcInside[0] == 1 && pos >= tContext->lowAngle && tContext->lowPos > tContext->segCount))
	{
		tContext->lowPos = tContext->segCount;	/* low angle is between low pos and low pos - 1 */
	}
	if ((tContext->arcInside[1] == 1 && pos >= tContext->highAngle && tContext->highPos > tContext->segCount) ||
		(tContext->arcInside[1] == 0 && pos <= tContext->highAngle && tContext->highPos < tContext->segCount))
	{
		tContext->highPos = tContext->segCount;	/* high angle is between high pos and high pos - 1 */
	}

	if (needToGo)
	{
        tContext->segments[tContext->segCount].flags = POINT_SMOOTH;
        tContext->segments[tContext->segCount++].coord = p2[0];
	}

	/* Check whether need to go down further to the back half */
	needToGo = gcvTRUE;
	switch (tContext->arcType)
	{
		case 0: 		/* tContext->segments end at high angle */
		case 1:
			if (pos > tContext->highAngle)
			{
				needToGo = gcvFALSE;
			}
			break;
		case 2: 		/* segements end at 1 */
		case 3:
			break;
	}
	if (needToGo)
	{
		if ((tContext->direction == 1 && pos + TESS_MUL(TESS_CONST_2, inc) > tContext->iterations[i+1]) ||
            (tContext->direction == -1 && TESS_CONST_ONE - pos > tContext->iterations[i+1]))
		{
			_DivideCircle(tContext, p2, i + 1, pos + inc, TESS_DIV(inc, TESS_CONST_2));
		}
	}
}

/* calculating steps needed in Bezier flattening. The convergence rate is
 approximately quadratic, therefore the /= 4 */
gctINT32 _StepsNeeded(
	_VGTessellationContext	*tContext,
    _VGVector2		p[]
    )
{
    gctINT32		steps = 0;
    _VGTesstype		len;

    _VGTesstype dist;
    _VGTesstype dx0, dy0, dx1, dy1;
    _VGTesstype x, y;

    x = (p[0].x + (p[1].x + p[2].x) * 3.0f + p[3].x) * 0.125f;
    y = (p[0].y + (p[1].y + p[2].y) * 3.0f + p[3].y) * 0.125f;
    dx0 = x - p[0].x;
    dy0 = y - p[0].y;
    dx1 = p[3].x - p[0].x;
    dy1 = p[3].y - p[0].y;

	if (dx1 != 0 || dy1 != 0)
	{
		dist = dx0 * dy1 - dx1 * dy0;
		dist *= dist;
		len = (dx1 * dx1 + dy1 * dy1) * tContext->epsilonSquare;
		while (dist > len)
		{
			dist *= 0.0625f;
			++steps;
		}
	}
	else
	{
		return 1;
	}
    /* Check the tangent angle epsilon (curvature). If the angle changes too much, we need to keep on dividing. */
	if (steps == 0 && tContext->flattenForStroke && tContext->strokeScale * tContext->strokeWidth >= tContext->strokeError)
    {
        dx0 = p[1].x - p[0].x;
        dy0 = p[1].y - p[0].y;

		dx1 = (p[2].x + p[3].x - p[0].x - p[1].x) * 0.25f;
		dy1 = (p[2].y + p[3].y - p[0].y - p[1].y) * 0.25f;

		if (dx0 * dx1 + dy0 * dy1 < 0.0f)	/* angle is in the second quartor. */
		{
			return 1;
		}
		else
		{
			dist = dx0 * dy1 - dy0 * dx1;
			if (dist * dist > tContext->angleEpsilon * (dx0*dx0 + dy0*dy0) * (dx1*dx1 + dy1*dy1) &&
				(dx0 != 0 || dy1 != 0) )
			{
				return 1;
			}
			else
			{
				dx0 = p[3].x - p[2].x;
				dy0 = p[3].y - p[2].y;

				if (dx0 * dx1 + dy0 * dy1 < 0)
				{
					return 1;
				}
				else
				{
					dist = dx0 * dy1 - dy0 * dx1;
					if (dist * dist > tContext->angleEpsilon * (dx0*dx0 + dy0*dy0) * (dx1*dx1 + dy1*dy1) &&
						(dx0 != 0 || dy1 != 0) )
					{
						return 1;
					}
				}
			}
		}
    }

    return  steps;
}

_VGTesstype _MockDistance(
    _VGVector2	p[]
    )
{
    _VGTesstype x1, y1, x2, y2, x3, y3, d, d1, d2;
    _VGTesstype ax1;
    _VGTesstype ay1;
	_VGTesstype ret;

	gcmHEADER_ARG("p=0x%x", p);

    x1 = p[3].x - p[0].x;
    y1 = p[3].y - p[0].y;
    x2 = p[1].x - p[0].x;
    y2 = p[1].y - p[0].y;
    x3 = p[2].x - p[3].x;
    y3 = p[2].y - p[3].y;

    ax1 = TESS_ABS(x1);
    ay1 = TESS_ABS(y1);

    d = ((ax1 > ay1) ? (ax1 + TESS_DIV(TESS_MUL(TESS_CONST_0_43, TESS_MUL(ay1, ay1)), ax1)) :
                       (ay1 + TESS_DIV(TESS_MUL(TESS_CONST_0_43, TESS_MUL(ax1, ax1)), ay1)));

    d1 = TESS_DIV((TESS_MUL(x1, y2) - TESS_MUL(x2, y1)), d);
    d2 = TESS_DIV((TESS_MUL(x1, y3) - TESS_MUL(x3, y1)), d);

    if (d1 > 0 && d2 > 0)		/* CPs on same side of base line */
    {
	    ret =  ((d1 > d2) ? TESS_MUL(d1, TESS_CONST_HALF) + TESS_MUL(d2, TESS_CONST_ONE_FOURTH) :
                            TESS_MUL(d1, TESS_CONST_ONE_FOURTH) + TESS_MUL(d2, TESS_CONST_HALF));
    }
    else if (d1 < 0 && d2 < 0)
    {
	    ret =  ((d1 > d2) ? TESS_MUL(-d1, TESS_CONST_ONE_FOURTH) - TESS_MUL(d2, TESS_CONST_HALF) :
                            TESS_MUL(-d1, TESS_CONST_HALF) - TESS_MUL(d2, TESS_CONST_ONE_FOURTH));
    }
    else if (d1 > 0 && d2 < 0)	/* CPs on different sides of base line */
    {
	    ret =  ((d1 > -d2) ? TESS_MUL(d1, TESS_CONST_HALF) :
                             TESS_MUL(-d2, TESS_CONST_HALF));
    }
    else
    {
	    ret =  ((-d1 > d2) ? TESS_MUL(-d1, TESS_CONST_HALF) :
                             TESS_MUL(d2, TESS_CONST_HALF));
    }

	gcmFOOTER_ARG("return=%f", ret);

	return ret;
}

_VGTesstype _MockLog2(
    _VGTesstype x
    )
{
	_VGTesstype ret = 0.0f;

	gcmHEADER_ARG("x=%f", x);

	if (x >= TESS_CONST_ONE)
	{
		gctINT32 i = 0;
		while (x >= TESS_CONST_2)
		{
			++i;
			x = TESS_MUL(x, TESS_CONST_HALF);
		}
		x -= TESS_CONST_ONE;
		/* Mock log base 2 for 1 < x < 2 by a quadratic curve */
		ret = TESSTYPE(i) + TESS_MUL(x, (TESS_CONST_1_3465 - TESS_MUL(TESS_CONST_0_3465, x)));
	}

	gcmFOOTER_ARG("return=%f", ret);

	return ret;
}

_VGTesstype _MockCubeRoot(
    _VGTesstype x
    )
{
	_VGTesstype ret;

	gcmHEADER_ARG("x=%f", x);

	/* Mocking cube root for 0 < x < 1 using three different functions. */
	if (x > TESS_CONST_0_3)
	{
		ret = (TESS_MUL((TESS_MUL(TESS_CONST_N0_25733, x) + TESS_CONST_0_80676), x) + TESS_CONST_0_45056);
	}else
    if (x > TESS_CONST_0_1)
	{
		ret = (TESS_MUL((TESS_MUL(TESS_CONST_N1_80076, x) + TESS_CONST_1_74668), x) + TESS_CONST_0_3075);
	}
	else
	{
		_VGTesstype x1 = TESS_SQRT(x);
		ret = TESS_MUL((x1 + TESS_MUL(TESS_CONST_1_08, TESS_SQRT(x1))), TESS_CONST_HALF);
	}

	gcmFOOTER_ARG("return=%f", ret);

	return ret;
}

_VGTesstype _GetS8 (
    _VGint8 *data
    )
{
    return *data;
}

_VGTesstype _GetS16(
    _VGint8 *data
    )
{
    return *((_VGint16*)data);
}

_VGTesstype _GetS32(
    _VGint8 *data
    )
{
	return (gctFLOAT)(*((gctINT32*)data));
}

_VGTesstype _GetF  (
    _VGint8 *data
    )
{
    return *((gctFLOAT*)data);
}

/* Approximate a path by line tContext->segments, whose vertices are stored in "points". */
gctINT32 _FlattenPath(
	_VGContext			*context,
    _VGPath             *path,
    _VGTesstype			strokeWidth,
    _VGFlattenBuffer    *flattened
    )
{
    gctINT32            i, j, k;                /* General loop counter. */
    gctINT32            numPoints;              /* The total number of all points. */
    _VGVector2			currentPoints[4];       /* The points in the segment just processed. up to 4 when in cubic beziers. */
    _VGVector2			sPoint, pPoint, oPoint;			/* As the spec suggests. */
    _VGint8             *dataPointer = gcvNULL;           /* The pointer to the point data of the VGPath.*/

    gctINT32            numCommands;            /* The number of commands in the path. */
    _VGint8             pathCommand;            /* Path command of each segment in the VGPath. */
	_VGint8				prevCommand;			/* The previous command when iterating a path. */
	gctBOOL				isRelative;
#if !USE_FTA
    _VGTessPoint        **pointsInSegment = gcvNULL;      /* An array of pointers. Which points to the result that the segment is flattened into. Remember to free before return.*/
    gctINT32            *numPointsInSegment = gcvNULL;    /* An array indicating how many points in a flattened segment. Remember to free before return.*/
    gctINT32            *numSegsInSubPath = gcvNULL;      /* An array indicating how many tContext->segments in a sub path. Remember to free before return.*/
#endif
    gctINT32            numSegments;            /* How many flattened tContext->segments in the path. */
    gctINT32            dataTypeSize;           /* What's the mem size of the data type? */
    gctINT32            currentSubPath;         /* What sub-path are we operating on? */
    gctINT32            currentSegment;         /* The segment's index we are processing. */

    gctINT32            segmentCounter;
	_VGVector2			cp[4];
    _VGEllipse          ellipse;
    _VGTessPoint        *pointAddr = gcvNULL;
    _VGTesstype         t0, t1, t2;

    _VGTessPoint        **points;
    gctINT32            **numPointsInSubPaths;
    gctINT32            *numSubPaths;
    gctBOOL             **isClosed;
    _ValueGetterFloat   getValue = gcvNULL;                  /* A funcion pointer to read a value from a general memory address. */
    _VGPathSegInfoArray *segsInfo = gcvNULL;

    _VGTesstype         scale;
    _VGTesstype         ellipseScale = TESS_CONST_ONE;
    _VGTesstype         pathBias = TESS_CONST_ZERO, pathScale = TESS_CONST_ONE;

	_VGTessHighType		tempValue;

	/* Variables inside a loop. */
    _VGTessPoint		*pointFirst, *pointLast;
    _VGVector2			oldTan, curTan;
    gctINT32			tan0Ready, prevFlag;		/* Indicate the tan0 is calculate from curve. */
	gctBOOL				pathQuery;

#if USE_FTA
	_VGFlattenContext	*fContext = &context->flatContext;
	gcoOS				os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x path=0x%x strokeWidth=%f flattened=0x%x",
				  context, path, strokeWidth, flattened);

	/* Handle an empty path. */
    if (path->segments.size < 1)
    {
		gcmFOOTER_ARG("return=%d", 0);
        return 0;
    }

	pathQuery = gcvTRUE;
	if (strokeWidth < 0)
	{
		strokeWidth = -strokeWidth;
	}
	else
	{
		pathQuery = gcvFALSE;
	}

	/* Select the data picker. */
    switch (path->datatype)
    {
    case VG_PATH_DATATYPE_S_8:
        getValue = _GetS8;
        dataTypeSize = 1;
        break;

    case VG_PATH_DATATYPE_S_16:
        getValue = _GetS16;
        dataTypeSize = 2;
        break;

    case VG_PATH_DATATYPE_S_32:
        getValue = _GetS32;
        dataTypeSize = 4;
        break;

    case VG_PATH_DATATYPE_F:
        getValue = _GetF;
        dataTypeSize = 4;
        break;

    default:
        gcmASSERT(0);
		gcmFOOTER_ARG("return=%d", -1);
        return -1;
    }

	/* Clean the container. */
    _VGFlattenBufferDtor(context->os, flattened);

    /* Do some initial settings. */
    numPointsInSubPaths = &flattened->numPointsInSubPath;
    points			= &flattened->points;
    numSubPaths		= &flattened->numSubPaths;
    isClosed		= &flattened->subPathClosed;
    segsInfo		= &path->segmentsInfo;
    dataPointer		= (_VGint8*)path->data.items;
    numSegments     = 0;
    numPoints       = 0;
    currentSubPath  = -1;
    currentSegment  = -1;
    *numSubPaths    = 0;
    numCommands     = path->segments.size;
    pathBias        = TESSTYPE(path->bias);
    pathScale       = TESSTYPE(path->scale);

	prevCommand		= VG_CLOSE_PATH;
	/* Count subpaths and segments (which have real lines). */
    for (i = 0; i < numCommands; ++i)
    {
        pathCommand = path->segments.items[i] & 0x1e;
		switch (pathCommand)
		{
		case VG_MOVE_TO:
			{
				/* Any move_to command leads a new path. */
				++(*numSubPaths);
			}
			break;
		default:
			{
				if (prevCommand == VG_CLOSE_PATH)
				{
					/* This leads to an implicit subpath beginning. */
					++(*numSubPaths);
				}
				++numSegments;
			}
			break;
		}

		prevCommand = pathCommand;
    }
	pathCommand = path->segments.items[0] & 0x1e;

    /* The memory address from the path can be reused here sometimes, I think. */
#if USE_FTA
	TA_Resize(os, (void**)&fContext->pointsInSegment, sizeof(_VGTessPoint*) * numSegments, 2);
	TA_Resize(os, (void**)&fContext->numPointsInSegment, sizeof(gctINT32) * numSegments, 1);
	TA_Resize(os, (void**)&fContext->numSegsInSubPath, sizeof(gctINT32) * (*numSubPaths), 1);
#else
	OVG_MALLOC(context->os, pointsInSegment, sizeof(_VGTessPoint*) * numSegments);
    OVG_MEMSET(pointsInSegment, 0, sizeof(_VGTessPoint*) * numSegments);

	OVG_MALLOC(context->os, numPointsInSegment, sizeof(gctINT32) * numSegments);
    OVG_MEMSET(numPointsInSegment, 0, sizeof(gctINT32) * numSegments);

	OVG_MALLOC(context->os, numSegsInSubPath, sizeof(gctINT32) * (*numSubPaths));
    OVG_MEMSET(numSegsInSubPath, 0, sizeof(gctINT32) * (*numSubPaths));
#endif

    OVG_SAFE_FREE(context->os, *numPointsInSubPaths);
	OVG_MALLOC(context->os, *numPointsInSubPaths, sizeof(gctINT32) * (*numSubPaths));
    OVG_MEMSET(*numPointsInSubPaths, 0, sizeof(gctINT32) * (*numSubPaths));

    OVG_SAFE_FREE(context->os, *isClosed);
	OVG_MALLOC(context->os, *isClosed, sizeof(gctBOOL) * (*numSubPaths));
    OVG_MEMSET(*isClosed, 0, sizeof(gctBOOL) * (*numSubPaths));

    segsInfo->size      = sizeof(_VGPathSegInfo) * numCommands;
    segsInfo->allocated = segsInfo->size;
    OVG_SAFE_FREE(context->os, segsInfo->items);
	OVG_MALLOC(context->os, segsInfo->items, segsInfo->allocated);
    OVG_MEMSET(segsInfo->items, 0, segsInfo->allocated);

    /* Initialize these variables in case the first command is not VG_MOVE_TO. */
    currentPoints[3].x = pathBias;
    currentPoints[3].y = pathBias;
    currentPoints[2] = currentPoints[3];
	sPoint = oPoint = pPoint = currentPoints[3];

    if (pathCommand != VG_MOVE_TO )
    {
        ++currentSubPath;
        (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
        fContext->numSegsInSubPath[currentSubPath] = 0;
#else
        numSegsInSubPath[currentSubPath] = 0;
#endif
        ++numPoints;       /* Inc the total count of points of the VGPath. */
    }

    /* Determine the epsilon for flattening curves. */
	scale = context->tessContext.strokeScale;
	if (scale == 0)
	{
		gcmFOOTER_ARG("return=%d", 0);
		return 0;
	}
    if (strokeWidth > 0)
        ellipseScale = TESS_MUL(gcmMAX(scale,  strokeWidth), TESS_CONST_ONE);
    else
        ellipseScale = scale;
	_SetEpsilonScale(&context->tessContext, scale);

	context->tessContext.strokeWidth = strokeWidth;
	context->tessContext.strokeError = context->strokeDashPattern.size > 0 ? 1.0f : 4.0f;

	prevCommand = VG_MOVE_TO;
    for (i = 0; i < numCommands; ++i)
    {
        pathCommand = (path->segments.items[i] & 0x1e);
		isRelative  = (path->segments.items[i] & 0x01);
		segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
		segsInfo->items[i].segShape.type = pathCommand;

        switch (pathCommand)
        {
        case VG_CLOSE_PATH:
            {
                (*isClosed)[currentSubPath] = gcvTRUE;
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = 2;
					V2_SETV(segsInfo->items[i].segShape.object.close.points[0], oPoint);
					V2_SETV(segsInfo->items[i].segShape.object.close.points[1], sPoint);
					segsInfo->items[i].segShape.fromP = segsInfo->items[i].segShape.object.close.points;
					segsInfo->items[i].segShape.toP = segsInfo->items[i].segShape.object.close.points + 1;
				}

                if (prevCommand == VG_MOVE_TO)
                {
                    ++currentSegment;
#if USE_FTA
					++fContext->numSegsInSubPath[currentSubPath];
                    fContext->numPointsInSegment[currentSegment] = 1;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint), 1);
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    /* Get points coordinate from data pointer. */
                    fContext->pointsInSegment[currentSegment][0].coord = sPoint;
#else
					++numSegsInSubPath[currentSubPath];
                    numPointsInSegment[currentSegment] = 1;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint));
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    /* Get points coordinate from data pointer. */
                    pointsInSegment[currentSegment][0].coord = sPoint;
#endif
                }
				else
				{
					if (pathQuery)
					{
						if (!VECTOR_EQUAL(sPoint, oPoint))		/* Get the exact tangents */
						{
							tempValue = (_VGTesstype)TESS_DIAG(sPoint.x - oPoint.x, sPoint.y - oPoint.y);
							segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = (sPoint.x - oPoint.x) / tempValue;
							segsInfo->items[i].inTan.y = segsInfo->items[i].outTan.y = (sPoint.y - oPoint.y) / tempValue;
						}
					}
				}

				oPoint = pPoint = sPoint;
            }
            break;

        case VG_MOVE_TO:        /* Indicate the beginning of a new sub-path. */
            {
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;     dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;     dataPointer += dataTypeSize;

                if (isRelative)
                {
					currentPoints[3].x += oPoint.x;
					currentPoints[3].y += oPoint.y;
                }
				sPoint = pPoint = oPoint = currentPoints[3];

                if ((i == 0)||
					(i > 0 && prevCommand != VG_MOVE_TO))	/* Continuous VG_MOVE_TOs are taken as empth subpaths. */
                {
                    ++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;       /* Inc the total count of points of the VGPath.*/
                }

				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = 0;
					V2_SETV(segsInfo->items[i].segShape.object.move.point, sPoint);
					segsInfo->items[i].segShape.fromP = segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.move.point;
				}
            }
            break;

        case VG_LINE_TO:   /* Create a new point set, and add the 2 points to the set.*/
		   {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

				currentPoints[0] = oPoint;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;
                if (isRelative)
                {
					currentPoints[3].x += oPoint.x;
					currentPoints[3].y += oPoint.y;
                }

				oPoint = pPoint = currentPoints[3];

                /* Gather information for this segment. */
                ++currentSegment;
#if USE_FTA
                fContext->numPointsInSegment[currentSegment] = 2;
				TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
				fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;

                /* Get points coordinate from data pointer. */
                fContext->pointsInSegment[currentSegment][0].coord = currentPoints[0];
                fContext->pointsInSegment[currentSegment][1].coord = currentPoints[3];
#else
                numPointsInSegment[currentSegment] = 2;
				OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                pointsInSegment[currentSegment][0].flags = POINT_FLAT;
				pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                pointsInSegment[currentSegment][1].flags = POINT_FLAT;

                /* Get points coordinate from data pointer. */
                pointsInSegment[currentSegment][0].coord = currentPoints[0];
                pointsInSegment[currentSegment][1].coord = currentPoints[3];
#endif

				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = 2;
					if (!VECTOR_EQUAL(currentPoints[0], currentPoints[3]))
					{
						tempValue = (_VGTesstype)TESS_DIAG(currentPoints[3].x - currentPoints[0].x, currentPoints[3].y - currentPoints[0].y);
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = (currentPoints[3].x - currentPoints[0].x) / tempValue;
						segsInfo->items[i].inTan.y = segsInfo->items[i].outTan.y = (currentPoints[3].y - currentPoints[0].y) / tempValue;
					}
					V2_SETV(segsInfo->items[i].segShape.object.line.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.line.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = segsInfo->items[i].segShape.object.line.points;
					segsInfo->items[i].segShape.toP = segsInfo->items[i].segShape.object.line.points + 1;
				}
                ++numPoints;

                ++(*numPointsInSubPaths)[currentSubPath];
#if USE_FTA
                ++fContext->numSegsInSubPath[currentSubPath];
#else
                ++numSegsInSubPath[currentSubPath];
#endif

            }
            break;

        case VG_HLINE_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

				currentPoints[0] = oPoint;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;     dataPointer += dataTypeSize;
                if (isRelative)
                {
					currentPoints[3].x += oPoint.x;
                }
				currentPoints[3].y = oPoint.y;

				oPoint.x = currentPoints[3].x;
				pPoint.x = currentPoints[3].x;
				pPoint.y = oPoint.y;

                /* Gather information for this segment. */
                ++currentSegment;
#if USE_FTA
                fContext->numPointsInSegment[currentSegment] = 2;
				TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
				fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;

                /* Get points coordinate from data pointer. */
                fContext->pointsInSegment[currentSegment][0].coord = currentPoints[0];
                fContext->pointsInSegment[currentSegment][1].coord = currentPoints[3];
#else
                numPointsInSegment[currentSegment] = 2;
				OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                pointsInSegment[currentSegment][0].flags = POINT_FLAT;
				pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                pointsInSegment[currentSegment][1].flags = POINT_FLAT;

                /* Get points coordinate from data pointer. */
                pointsInSegment[currentSegment][0].coord = currentPoints[0];
                pointsInSegment[currentSegment][1].coord = currentPoints[3];
#endif
                currentPoints[2] = currentPoints[3];

				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = 2;
					V2_SETV(segsInfo->items[i].segShape.object.line.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.line.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = segsInfo->items[i].segShape.object.line.points;
					segsInfo->items[i].segShape.toP = segsInfo->items[i].segShape.object.line.points + 1;

					if (!VECTOR_EQUAL(currentPoints[0], currentPoints[3]))
					{
						if (currentPoints[0].x < currentPoints[3].x)
						{
							segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = TESS_CONST_ONE;
						}
						else
						{
							segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = -TESS_CONST_ONE;
						}
						segsInfo->items[i].inTan.y = segsInfo->items[i].outTan.y = 0;
					}
				}
                ++numPoints;

                ++(*numPointsInSubPaths)[currentSubPath];
#if USE_FTA
                ++fContext->numSegsInSubPath[currentSubPath];
#else
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_VLINE_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

				currentPoints[0] = oPoint;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;     dataPointer += dataTypeSize;
                if (isRelative)
                {
					currentPoints[3].y += oPoint.y;
                }
				currentPoints[3].x = oPoint.x;

				pPoint.x = oPoint.x;
				pPoint.y = currentPoints[3].y;
				oPoint.y = currentPoints[3].y;

                /* Gather information for this segment. */
                ++currentSegment;
#if USE_FTA
                fContext->numPointsInSegment[currentSegment] = 2;
				TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
				fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;

                /* Get points coordinate from data pointer. */
                fContext->pointsInSegment[currentSegment][0].coord = currentPoints[0];
                fContext->pointsInSegment[currentSegment][1].coord = currentPoints[3];
#else
                numPointsInSegment[currentSegment] = 2;
				OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                pointsInSegment[currentSegment][0].flags = POINT_FLAT;
				pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                pointsInSegment[currentSegment][1].flags = POINT_FLAT;

                /* Get points coordinate from data pointer. */
                pointsInSegment[currentSegment][0].coord = currentPoints[0];
                pointsInSegment[currentSegment][1].coord = currentPoints[3];
#endif
                currentPoints[2] = currentPoints[3];

				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = 2;
					V2_SETV(segsInfo->items[i].segShape.object.line.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.line.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = segsInfo->items[i].segShape.object.line.points;
					segsInfo->items[i].segShape.toP = segsInfo->items[i].segShape.object.line.points + 1;

					if (!VECTOR_EQUAL(currentPoints[0], currentPoints[3]))
					{
						if (currentPoints[0].y < currentPoints[3].y)
						{
							segsInfo->items[i].inTan.y = segsInfo->items[i].outTan.y = TESS_CONST_ONE;
						}
						else
						{
							segsInfo->items[i].inTan.y = segsInfo->items[i].outTan.y = -TESS_CONST_ONE;
						}
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = 0;
					}
				}
                ++numPoints;

                ++(*numPointsInSubPaths)[currentSubPath];
#if USE_FTA
                ++fContext->numSegsInSubPath[currentSubPath];
#else
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_QUAD_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                /* Get control points data. */
                currentPoints[0] = oPoint;
                currentPoints[2].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                    dataPointer += dataTypeSize;
                currentPoints[2].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                    dataPointer += dataTypeSize;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                    dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                    dataPointer += dataTypeSize;

                if (isRelative)
                {
                    currentPoints[2].x += oPoint.x;
                    currentPoints[2].y += oPoint.y;
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint = currentPoints[2];
				oPoint = currentPoints[3];

                /* Convert it to cubic bezier, and flatten it. */
                cp[0] = currentPoints[0];
                cp[3] = currentPoints[3];
				/* Remove some math error on arm. */
				if (VECTOR_EQUAL(cp[0], currentPoints[2]))
				{
					cp[1] = cp[0];
				}
				else
				{
					cp[1].x = TESS_DIV((currentPoints[0].x + TESS_MUL(TESS_CONST_2, currentPoints[2].x)), 3.0f);
					cp[1].y = TESS_DIV((currentPoints[0].y + TESS_MUL(TESS_CONST_2, currentPoints[2].y)), 3.0f);
				}
                cp[2].x = TESS_DIV((currentPoints[3].x + TESS_MUL(TESS_CONST_2, currentPoints[2].x)), 3.0f);
                cp[2].y = TESS_DIV((currentPoints[3].y + TESS_MUL(TESS_CONST_2, currentPoints[2].y)), 3.0f);
                ++currentSegment;

                if ( VECTOR_EQUAL(cp[0], cp[1]) &&
                     VECTOR_EQUAL(cp[0], cp[2]) &&
                     VECTOR_EQUAL(cp[0], cp[3]))
                {
					/* The whole bezier shrinks to a single point. */
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = oPoint;
                    fContext->pointsInSegment[currentSegment][1].coord = oPoint;
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = oPoint;
                    pointsInSegment[currentSegment][1].coord = oPoint;
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _SetStrokeFlatten(&context->tessContext, strokeWidth);
#if USE_FTA
					fContext->numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &fContext->pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetStrokeFlatten(&context->tessContext, 0);
                }

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier.cp[0] = cp[0];
					segsInfo->items[i].segShape.object.bezier.cp[1] = cp[1];
					segsInfo->items[i].segShape.object.bezier.cp[2] = cp[2];
					segsInfo->items[i].segShape.object.bezier.cp[3] = cp[3];
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}
                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier = bezier;
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}
                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_CUBIC_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                /* Get control points data. */
                currentPoints[0] = oPoint;
                for (j = 1; j < 4; ++j)
                {
                    currentPoints[j].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                    dataPointer += dataTypeSize;
                    currentPoints[j].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                    dataPointer += dataTypeSize;
                }

                if (isRelative)
                {
                    currentPoints[1].x += oPoint.x;
                    currentPoints[1].y += oPoint.y;
                    currentPoints[2].x += oPoint.x;
                    currentPoints[2].y += oPoint.y;
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint = currentPoints[2];
				oPoint = currentPoints[3];

                /* Flatten the bezier, and get the information. */
                cp[0] = currentPoints[0];
                cp[1] = currentPoints[1];
                cp[2] = currentPoints[2];
                cp[3] = currentPoints[3];

                ++currentSegment;

                if ( VECTOR_EQUAL(cp[0], cp[1]) &&
                     VECTOR_EQUAL(cp[0], cp[2]) &&
                     VECTOR_EQUAL(cp[0], cp[3]))
                {
					/* The whole bezier shrinks to a single point. */
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = oPoint;
                    fContext->pointsInSegment[currentSegment][1].coord = oPoint;
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = oPoint;
                    pointsInSegment[currentSegment][1].coord = oPoint;
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _SetStrokeFlatten(&context->tessContext, strokeWidth);
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &fContext->pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetStrokeFlatten(&context->tessContext, TESS_CONST_ZERO);
                }

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier.cp[0] = cp[0];
					segsInfo->items[i].segShape.object.bezier.cp[1] = cp[1];
					segsInfo->items[i].segShape.object.bezier.cp[2] = cp[2];
					segsInfo->items[i].segShape.object.bezier.cp[3] = cp[3];
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}
                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier = bezier;
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}
                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_SQUAD_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                /* Get control points data. */
                currentPoints[0]   = oPoint;
				/* The implicit point. */
                currentPoints[2].x = oPoint.x * 2 - pPoint.x;
                currentPoints[2].y = oPoint.y * 2 - pPoint.y;

                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;

                if (isRelative)
                {
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint.x = oPoint.x * 2.0f - pPoint.x;
				pPoint.y = oPoint.y * 2.0f - pPoint.y;
				oPoint = currentPoints[3];

                /* Convert it to cubic bezier, and flatten it. */
                cp[0] = currentPoints[0];
                cp[3] = currentPoints[3];
				/* Remove some math error on arm. */
				if (VECTOR_EQUAL(cp[0], currentPoints[2]))
				{
					cp[1] = cp[0];
				}
				else
				{
					cp[1].x = TESS_DIV((currentPoints[0].x + TESS_MUL(TESS_CONST_2, currentPoints[2].x)), 3.0f);
					cp[1].y = TESS_DIV((currentPoints[0].y + TESS_MUL(TESS_CONST_2, currentPoints[2].y)), 3.0f);
				}
                cp[2].x = TESS_DIV((currentPoints[3].x + TESS_MUL(TESS_CONST_2, currentPoints[2].x)), 3.0f);
                cp[2].y = TESS_DIV((currentPoints[3].y + TESS_MUL(TESS_CONST_2, currentPoints[2].y)), 3.0f);

                ++currentSegment;

                if ( VECTOR_EQUAL(cp[0], cp[1]) &&
                     VECTOR_EQUAL(cp[0], cp[2]) &&
                     VECTOR_EQUAL(cp[0], cp[3]))
                {
					/* The whole bezier shrinks to a single point. */
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = oPoint;
                    fContext->pointsInSegment[currentSegment][1].coord = oPoint;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = oPoint;
                    pointsInSegment[currentSegment][1].coord = oPoint;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _SetStrokeFlatten(&context->tessContext, strokeWidth);
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &fContext->pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetStrokeFlatten(&context->tessContext, 0);
                }

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier.cp[0] = cp[0];
					segsInfo->items[i].segShape.object.bezier.cp[1] = cp[1];
					segsInfo->items[i].segShape.object.bezier.cp[2] = cp[2];
					segsInfo->items[i].segShape.object.bezier.cp[3] = cp[3];
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}

                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier = bezier;
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}

                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_SCUBIC_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                /* Get control points data. */
                currentPoints[0]   = oPoint;
				/* The implicit point. */
                currentPoints[1].x = oPoint.x * 2 - pPoint.x;
                currentPoints[1].y = oPoint.y * 2 - pPoint.y;
                currentPoints[2].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;
                currentPoints[2].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;  dataPointer += dataTypeSize;

                if (isRelative)
                {
                    currentPoints[2].x += oPoint.x;
                    currentPoints[2].y += oPoint.y;
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint = currentPoints[2];
				oPoint = currentPoints[3];

                /* Flatten the bezier, and get the information. */
                cp[0] = currentPoints[0];
                cp[1] = currentPoints[1];
                cp[2] = currentPoints[2];
                cp[3] = currentPoints[3];

                ++currentSegment;

                if ( VECTOR_EQUAL(cp[0], cp[1]) &&
                     VECTOR_EQUAL(cp[0], cp[2]) &&
                     VECTOR_EQUAL(cp[0], cp[3]))
                {
					/* The whole bezier shrinks to a single point. */
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = oPoint;
                    fContext->pointsInSegment[currentSegment][1].coord = oPoint;
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = oPoint;
                    pointsInSegment[currentSegment][1].coord = oPoint;
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _SetStrokeFlatten(&context->tessContext, strokeWidth);
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &fContext->pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _BezierFlatten(context, cp, &pointsInSegment[currentSegment]);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetStrokeFlatten(&context->tessContext, 0);
                }

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier.cp[0] = cp[0];
					segsInfo->items[i].segShape.object.bezier.cp[1] = cp[1];
					segsInfo->items[i].segShape.object.bezier.cp[2] = cp[2];
					segsInfo->items[i].segShape.object.bezier.cp[3] = cp[3];
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}

                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.bezier = bezier;
					segsInfo->items[i].segShape.fromP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[0];
					segsInfo->items[i].segShape.toP = (_VGVector2*)&segsInfo->items[i].segShape.object.bezier.cp[3];
				}

                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_SCCWARC_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                ++currentSegment;

                /* Setup the ellipse object for flattening. */
                ellipse.hAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);    dataPointer += dataTypeSize;
                ellipse.vAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);    dataPointer += dataTypeSize;
                t2 = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                         dataPointer += dataTypeSize;

                currentPoints[0] = oPoint;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;              dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;              dataPointer += dataTypeSize;


                if (isRelative)
                {
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint = oPoint = currentPoints[3];

                /* If either of the axises is 0, a line is generated instead of an arc. */
                if (VECTOR_EQUAL(currentPoints[0], currentPoints[3]) || (ellipse.hAxis == 0) || (ellipse.vAxis == 0))
                {
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    fContext->pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _CreateEllipse(ellipse.hAxis,
                                   ellipse.vAxis,
                                   TESS_MUL(t2 - (gctINT32)(t2/TESSTYPE(360.0f))*TESSTYPE(360.0f), TESS_PI_DIV_180),
                                   &currentPoints[0],
                                   &currentPoints[3],
                                   0,
                                   gcvTRUE,
                                   &t0,
                                   &t1,
                                   &ellipse);
					_SetEpsilonScale(&context->tessContext, ellipseScale);
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = _EllipseFlatten(context,
																		 &ellipse,
                                                                         t0,
                                                                         t1,
                                                                         &currentPoints[0],
                                                                         &currentPoints[3],
                                                                         &fContext->pointsInSegment[currentSegment],
																		 0);

					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _EllipseFlatten(context,
																		 &ellipse,
                                                                         t0,
                                                                         t1,
                                                                         &currentPoints[0],
                                                                         &currentPoints[3],
                                                                         &pointsInSegment[currentSegment],
																		 0);

					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetEpsilonScale(&context->tessContext, scale);
                }
                /* Update the current states for running through tContext->segments. */
                currentPoints[2] = currentPoints[3];

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}

                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}

                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_SCWARC_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                ++currentSegment;

                /* Setup the ellipse object for flattening. */
                ellipse.hAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);        dataPointer += dataTypeSize;
                ellipse.vAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);        dataPointer += dataTypeSize;
                t2 = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                   dataPointer += dataTypeSize;

                currentPoints[0] = oPoint;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                  dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                  dataPointer += dataTypeSize;

                if (isRelative)
                {
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint = oPoint = currentPoints[3];

				/* If either of the axises is 0, a line is generated instead of an arc. */
                if (VECTOR_EQUAL(currentPoints[0], currentPoints[3]) || (ellipse.hAxis == 0) || (ellipse.vAxis == 0))
                {
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    fContext->pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _CreateEllipse(ellipse.hAxis,
                                   ellipse.vAxis,
                                   TESS_MUL(t2 - (gctINT32)(t2/TESSTYPE(360.0f))*TESSTYPE(360.0f), TESS_PI_DIV_180),
                                   &currentPoints[3],
                                   &currentPoints[0],
                                   0,
                                   gcvTRUE,
                                   &t0,
                                   &t1,
                                   &ellipse);
                    _SetEpsilonScale(&context->tessContext, ellipseScale);
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = _EllipseFlatten(context,
																		&ellipse,
                                                                      t0,
                                                                      t1,
                                                                      &currentPoints[3],
                                                                      &currentPoints[0],
                                                                      &fContext->pointsInSegment[currentSegment],
																	  0);
                    _ReversePointArray(fContext->pointsInSegment[currentSegment],
                                       fContext->numPointsInSegment[currentSegment],
                                       gcvTRUE);  /* Our algorithm only do with CCW, so for CW we need to reverse the result. */
					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _EllipseFlatten(context,
																		&ellipse,
                                                                      t0,
                                                                      t1,
                                                                      &currentPoints[3],
                                                                      &currentPoints[0],
                                                                      &pointsInSegment[currentSegment],
																	  0);
                    _ReversePointArray(pointsInSegment[currentSegment],
                                       numPointsInSegment[currentSegment],
                                       gcvTRUE);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetEpsilonScale(&context->tessContext, scale);
                }

                /* Update the current states for running through tContext->segments. */
                currentPoints[2] = currentPoints[3];

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}
                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}
                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_LCCWARC_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                ++currentSegment;

                /* Setup the ellipse object for flattening. */
                ellipse.hAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);      dataPointer += dataTypeSize;
                ellipse.vAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);      dataPointer += dataTypeSize;
                t2 = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                           dataPointer += dataTypeSize;

                currentPoints[0] = oPoint;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                dataPointer += dataTypeSize;

                if (isRelative)
                {
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint = oPoint = currentPoints[3];

				/* If either of the axises is 0, a line is generated instead of an arc. */
                if (VECTOR_EQUAL(currentPoints[0], currentPoints[3]) || (ellipse.hAxis == 0) || (ellipse.vAxis == 0))
                {
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    fContext->pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _CreateEllipse(ellipse.hAxis,
                                   ellipse.vAxis,
                                   TESS_MUL(t2 - (gctINT32)(t2/TESSTYPE(360.0f))*TESSTYPE(360.0f), TESS_PI_DIV_180),
                                   &currentPoints[0],
                                   &currentPoints[3],
                                   1,
                                   gcvTRUE,
                                   &t0,
                                   &t1,
                                   &ellipse);
                    _SetEpsilonScale(&context->tessContext, ellipseScale);
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = _EllipseFlatten(context, &ellipse,
                                                                      t0,
                                                                      t1,
                                                                      &currentPoints[0],
                                                                      &currentPoints[3],
                                                                      &fContext->pointsInSegment[currentSegment],
																	  0);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _EllipseFlatten(context, &ellipse,
                                                                      t0,
                                                                      t1,
                                                                      &currentPoints[0],
                                                                      &currentPoints[3],
                                                                      &pointsInSegment[currentSegment],
																	  0);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetEpsilonScale(&context->tessContext, scale);
                }

                /* Update the current states for running through tContext->segments. */
                currentPoints[2] = currentPoints[3];

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}
                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}
                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        case VG_LCWARC_TO:
            {
				if (prevCommand == VG_CLOSE_PATH)	/* Implicitly begin a new subpath. The following block acts like a VG_MOVE_TO. */
				{
					++currentSubPath;
                    (*numPointsInSubPaths)[currentSubPath] = 1;
#if USE_FTA
                    fContext->numSegsInSubPath[currentSubPath] = 0;
#else
                    numSegsInSubPath[currentSubPath] = 0;
#endif
                    ++numPoints;
				}

                ++currentSegment;

                /* Setup the ellipse object for flattening. */
                ellipse.hAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);        dataPointer += dataTypeSize;
                ellipse.vAxis = TESS_ABS(TESS_MUL(getValue(dataPointer),  pathScale) + pathBias);        dataPointer += dataTypeSize;
                t2 = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                             dataPointer += dataTypeSize;

                currentPoints[0] = oPoint;
                currentPoints[3].x = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                  dataPointer += dataTypeSize;
                currentPoints[3].y = TESS_MUL(getValue(dataPointer),  pathScale) + pathBias;                  dataPointer += dataTypeSize;

                if (isRelative)
                {
                    currentPoints[3].x += oPoint.x;
                    currentPoints[3].y += oPoint.y;
                }

				pPoint = oPoint = currentPoints[3];

				/* If either of the axises is 0, a line is generated instead of an arc. */
                if (VECTOR_EQUAL(currentPoints[0], currentPoints[3]) || (ellipse.hAxis == 0) || (ellipse.vAxis == 0))
                {
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = 2;
					TA_Resize(os, (void**)&fContext->pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2, 1);
                    fContext->pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    fContext->pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    fContext->pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					fContext->pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    fContext->pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#else
                    numPointsInSegment[currentSegment] = 2;
					OVG_MALLOC(context->os, pointsInSegment[currentSegment], sizeof(_VGTessPoint) * 2);
                    pointsInSegment[currentSegment][0].coord = currentPoints[0];
                    pointsInSegment[currentSegment][1].coord = currentPoints[3];
                    pointsInSegment[currentSegment][0].flags = POINT_FLAT;
					pointsInSegment[currentSegment][0].inTan.x = UN_INIT_TAN_VALUE;
                    pointsInSegment[currentSegment][1].flags = POINT_FLAT;
#endif
					if (pathQuery)
					{
						segsInfo->items[i].inTan.x = segsInfo->items[i].outTan.x = UN_INIT_TAN_VALUE;
					}
                }
                else
                {
                    _CreateEllipse(ellipse.hAxis,
                                   ellipse.vAxis,
                                   TESS_MUL(t2 - (gctINT32)(t2/TESSTYPE(360.0f))*TESSTYPE(360.0f), TESS_PI_DIV_180),
                                   &currentPoints[3],
                                   &currentPoints[0],
                                   1,
                                   gcvTRUE,
                                   &t0,
                                   &t1,
                                   &ellipse);
                    _SetEpsilonScale(&context->tessContext, ellipseScale);
#if USE_FTA
                    fContext->numPointsInSegment[currentSegment] = _EllipseFlatten(context, &ellipse,
                                                                      t0,
                                                                      t1,
                                                                      &currentPoints[3],
                                                                      &currentPoints[0],
                                                                      &fContext->pointsInSegment[currentSegment],
																	  0);
                    _ReversePointArray(fContext->pointsInSegment[currentSegment],
                                       fContext->numPointsInSegment[currentSegment],
                                       gcvTRUE);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = fContext->pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = fContext->pointsInSegment[currentSegment][fContext->numPointsInSegment[currentSegment] - 1].inTan;
					}
#else
                    numPointsInSegment[currentSegment] = _EllipseFlatten(context, &ellipse,
                                                                      t0,
                                                                      t1,
                                                                      &currentPoints[3],
                                                                      &currentPoints[0],
                                                                      &pointsInSegment[currentSegment],
																	  0);
                    _ReversePointArray(pointsInSegment[currentSegment],
                                       numPointsInSegment[currentSegment],
                                       gcvTRUE);
					if (pathQuery)
					{
						segsInfo->items[i].inTan = pointsInSegment[currentSegment]->outTan;
						segsInfo->items[i].outTan = pointsInSegment[currentSegment][numPointsInSegment[currentSegment] - 1].inTan;
					}
#endif
                    _SetEpsilonScale(&context->tessContext, scale);
                }

                /* Update the current states for running through tContext->segments. */
                currentPoints[2] = currentPoints[3];

#if USE_FTA
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = fContext->numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}
                numPoints += fContext->numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += fContext->numPointsInSegment[currentSegment] - 1;
                ++fContext->numSegsInSubPath[currentSubPath];
#else
				if (pathQuery)
				{
					segsInfo->items[i].startPoint = numPoints - 1;
					segsInfo->items[i].numPoints = numPointsInSegment[currentSegment];
					segsInfo->items[i].segShape.object.arc.ellipse = ellipse;
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[0], currentPoints[0]);
					V2_SETV(segsInfo->items[i].segShape.object.arc.points[1], currentPoints[3]);
					segsInfo->items[i].segShape.fromP = &segsInfo->items[i].segShape.object.arc.points[0];
					segsInfo->items[i].segShape.toP = &segsInfo->items[i].segShape.object.arc.points[1];
				}
                numPoints += numPointsInSegment[currentSegment] - 1;

                (*numPointsInSubPaths)[currentSubPath] += numPointsInSegment[currentSegment] - 1;
                ++numSegsInSubPath[currentSubPath];
#endif
            }
            break;

        default:
            gcmASSERT(0);
			{
				_VGFlattenBufferDtor(context->os, flattened);

#if !USE_FTA
				OVG_SAFE_FREE(context->os, pointsInSegment);
				OVG_SAFE_FREE(context->os, numPointsInSegment);
				OVG_SAFE_FREE(context->os, numSegsInSubPath);
#endif

				gcmFOOTER_ARG("return=%d", 0);
				return 0;
			}
        }

		prevCommand = pathCommand;
    }

    if (numPoints < 1 || currentSegment < 0 || currentSubPath < 0)
    {
        _VGFlattenBufferDtor(context->os, flattened);

#if !USE_FTA
        OVG_SAFE_FREE(context->os, pointsInSegment);
        OVG_SAFE_FREE(context->os, numPointsInSegment);
        OVG_SAFE_FREE(context->os, numSegsInSubPath);
#endif

		gcmFOOTER_ARG("return=%d", 0);
        return 0;
    }
#ifdef TIMER_STATISTIC
    ot1 = GetTickCount();
#endif

#ifdef PERFORMANCE_STAT
	PerfRecordBegin(context->perfStat, "_FlatteningPath - Assemble");
#endif
    /* Copy results. */
	gcmASSERT(numSegments > currentSegment);
	gcmASSERT(*numSubPaths > currentSubPath);
    numSegments = currentSegment + 1;
    *numSubPaths = currentSubPath + 1;
    segmentCounter = 0;

	/* The memory from path can also be reused. */
    OVG_SAFE_FREE(context->os, *points);
	OVG_MALLOC(context->os, *points, sizeof(_VGTessPoint) * numPoints);
    if (*points == gcvNULL)
    {
        _VGFlattenBufferDtor(context->os, flattened);

#if !USE_FTA
        OVG_SAFE_FREE(context->os, pointsInSegment);
        OVG_SAFE_FREE(context->os, numPointsInSegment);
        OVG_SAFE_FREE(context->os, numSegsInSubPath);
#endif

		gcmFOOTER_ARG("return=%d", 0);
        return 0;
    }

    pointAddr = *points;
	if (strokeWidth == 0)
	{
#if USE_FTA
		_VGTessPoint	**pointsInSegment = fContext->pointsInSegment;
		gctINT32		*numPointsInSegment = fContext->numPointsInSegment;
		gctINT32		*numSegsInSubPath = fContext->numSegsInSubPath;
#endif
		for (i = 0; i < *numSubPaths; ++i)
		{
			if (numSegsInSubPath[i] <= 0)
				continue;

			pointFirst = pointAddr;
			OVG_MEMCOPY(pointAddr, pointsInSegment[segmentCounter], sizeof(_VGTessPoint) * numPointsInSegment[segmentCounter]);
			pointAddr += numPointsInSegment[segmentCounter];
			++segmentCounter;

			for (j = 1; j < numSegsInSubPath[i]; ++j)
			{
				OVG_MEMCOPY(pointAddr,
							pointsInSegment[segmentCounter] + 1,
							sizeof(_VGTessPoint) * (numPointsInSegment[segmentCounter] - 1));
				pointAddr += numPointsInSegment[segmentCounter] - 1;
				++segmentCounter;
			}

		}
	}
	else
	{
#if USE_FTA
		_VGTessPoint	**pointsInSegment = fContext->pointsInSegment;
		gctINT32		*numPointsInSegment = fContext->numPointsInSegment;
		gctINT32		*numSegsInSubPath = fContext->numSegsInSubPath;
#endif
		int ppCounter = 0;
		numPoints = 0;
		for (i = 0; i < *numSubPaths; ++i)
		{
			oldTan.x = TESS_CONST_ONE;
			oldTan.y = 0;
			tan0Ready = 0;
			prevFlag = 100;		/* Indicate the tan0 is calculate from curve. */

			if (numSegsInSubPath[i] <= 0)
			{
				ppCounter++;
				continue;
			}

			/* Calculate the inTangent of the points on the first segment. */
			if (pointsInSegment[segmentCounter][0].flags == POINT_FLAT)
			{
				pointsInSegment[segmentCounter][0].inTan.x = UN_INIT_TAN_VALUE;
				pointsInSegment[segmentCounter][0].inTan.y = 0;

			}
			if(pointsInSegment[segmentCounter][0].inTan.x != UN_INIT_TAN_VALUE)
			{
				oldTan = pointsInSegment[segmentCounter][0].inTan;
				tan0Ready = 1;
			}
			for (k = 0; k < numPointsInSegment[segmentCounter] - 1; ++k)
			{
				t2 = GetLineDirection(&pointsInSegment[segmentCounter][k].coord,
									  &pointsInSegment[segmentCounter][k+1].coord, &curTan);
				if (t2 <= 0)
				{
					t0 = oldTan.x;
					t1 = oldTan.y;
					t2 = 0;
				}
				else
				{
					t0 = curTan.x;
					t1 = curTan.y;
					oldTan.x = t0;
					oldTan.y = t1;
				}
				if(k && prevFlag != 100)
				{
					pointsInSegment[segmentCounter][k].flags = (_VGuint8) prevFlag;
					pointsInSegment[segmentCounter][k].outTan = pointsInSegment[segmentCounter][k-1].outTan;
				}
				if ((pointsInSegment[segmentCounter][k].flags == POINT_SMOOTH) ||
					(pointsInSegment[segmentCounter][k].flags == POINT_FLAT))
				{
					pointsInSegment[segmentCounter][k].outTan.x = t0;
					pointsInSegment[segmentCounter][k].outTan.y = t1;
				}
				if (pointsInSegment[segmentCounter][k + 1].flags != POINT_SMOOTH_IN)
				{
					pointsInSegment[segmentCounter][k + 1].inTan.x = t0;
					pointsInSegment[segmentCounter][k + 1].inTan.y = t1;
				}
				pointsInSegment[segmentCounter][k].distance2Next = t2;
				if(pointsInSegment[segmentCounter][k].coord.x == pointsInSegment[segmentCounter][k+1].coord.x &&
				   pointsInSegment[segmentCounter][k].coord.y == pointsInSegment[segmentCounter][k+1].coord.y)
					prevFlag = pointsInSegment[segmentCounter][k].flags;
				else
					prevFlag = 100;
			}

			/* First segment of the sub path is fully copied. */
			pointFirst = pointAddr;
			OVG_MEMCOPY(pointAddr, pointsInSegment[segmentCounter], sizeof(_VGTessPoint) * numPointsInSegment[segmentCounter]);
			pointAddr += numPointsInSegment[segmentCounter];
			numPoints += numPointsInSegment[segmentCounter];
			++segmentCounter;

			for (j = 1; j < numSegsInSubPath[i]; ++j)
			{
				/* Calculate the tangents of each point on the segment. */
				if(pointsInSegment[segmentCounter][0].inTan.x != UN_INIT_TAN_VALUE)
					oldTan = pointsInSegment[segmentCounter][0].inTan;
				for (k = 0; k < numPointsInSegment[segmentCounter] - 1; ++k)
				{
					t2 = GetLineDirection(&pointsInSegment[segmentCounter][k].coord, &pointsInSegment[segmentCounter][k+1].coord, &curTan);

					if (t2 <= 0)
					{
						t0 = oldTan.x;
						t1 = oldTan.y;
						t2 = 0;
					}
					else
					{
						t0 = curTan.x;
						t1 = curTan.y;
						oldTan.x = t0;
						oldTan.y = t1;
					}
					if(k && prevFlag != 100)
					{
						pointsInSegment[segmentCounter][k].flags = (_VGuint8) prevFlag;
						pointsInSegment[segmentCounter][k].outTan = pointsInSegment[segmentCounter][k-1].outTan;
					}
					if ((pointsInSegment[segmentCounter][k].flags == POINT_SMOOTH) ||
						(pointsInSegment[segmentCounter][k].flags == POINT_FLAT))

					{
						pointsInSegment[segmentCounter][k].outTan.x = t0;
						pointsInSegment[segmentCounter][k].outTan.y = t1;
					}
					if (pointsInSegment[segmentCounter][k + 1].flags != POINT_SMOOTH_IN)
					{
						pointsInSegment[segmentCounter][k + 1].inTan.x = t0;
						pointsInSegment[segmentCounter][k + 1].inTan.y = t1;
					}
					pointsInSegment[segmentCounter][k].distance2Next = t2;
					if(pointsInSegment[segmentCounter][k].coord.x == pointsInSegment[segmentCounter][k+1].coord.x &&
					   pointsInSegment[segmentCounter][k].coord.y == pointsInSegment[segmentCounter][k+1].coord.y)
						prevFlag = pointsInSegment[segmentCounter][k].flags;
					else
						prevFlag = 100;
				}
				(pointAddr - 1)->flags |= pointsInSegment[segmentCounter][0].flags;
				(pointAddr - 1)->outTan = pointsInSegment[segmentCounter][0].outTan;
				(pointAddr - 1)->distance2Next = pointsInSegment[segmentCounter][0].distance2Next;

				OVG_MEMCOPY(pointAddr,
							pointsInSegment[segmentCounter] + 1,
							sizeof(_VGTessPoint) * (numPointsInSegment[segmentCounter] - 1));
				numPoints += numPointsInSegment[segmentCounter] - 1;
				pointAddr += numPointsInSegment[segmentCounter] - 1;
				++segmentCounter;
			}

			if (pathQuery)
			{
				_VGVector2	lastTan = {TESS_CONST_ONE, 0};
				_VGTessPoint *itPoint = gcvNULL;
				itPoint = pointAddr - 1;
				for (;itPoint >= pointFirst; itPoint--)
				{
					if (itPoint->outTan.x == UN_INIT_TAN_VALUE)
					{
						itPoint->outTan = lastTan;
					}
					else
					if (TESS_ABS(itPoint->outTan.x) <= TESS_CONST_ONE &&
						TESS_ABS(itPoint->outTan.y) <= TESS_CONST_ONE)
					{
						lastTan = itPoint->outTan;
					}
				}

				lastTan.x = TESS_CONST_ONE;
				lastTan.y = 0;
				itPoint = pointFirst;
				for (;itPoint < pointAddr - 1; itPoint++)
				{
					if (itPoint->inTan.x == UN_INIT_TAN_VALUE)
					{
						itPoint->inTan = lastTan;
					}
					else
					if (TESS_ABS(itPoint->inTan.x) <= TESS_CONST_ONE &&
						TESS_ABS(itPoint->inTan.y) <= TESS_CONST_ONE)
					{
						lastTan = itPoint->inTan;
					}
				}
			}
			if ((*isClosed)[i])
			{
				pointLast = pointAddr - 1;
				t0 = pointFirst->coord.x - pointLast->coord.x;
				t1 = pointFirst->coord.y - pointLast->coord.y;
				t2 = (_VGTesstype)TESS_DIAG(t0, t1);
				if (t2 <= 0)
				{
					t0 = (oldTan.x);
					t1 = (oldTan.y);
					t2 = 0;
				}
				else
				{
					t0 = TESS_DIV(t0, t2);
					t1 = TESS_DIV(t1, t2);
					oldTan.x = (t0);
					oldTan.y = (t1);
				}
				if (pointFirst->flags == POINT_FLAT ||
					pointFirst->flags == POINT_SMOOTH_OUT)
				{
					if(tan0Ready == 0 || t2 > 0)
					{
						pointFirst->inTan.x = t0;
						pointFirst->inTan.y = t1;
					}
				}
				if (pointLast->flags == POINT_FLAT ||
					pointLast->flags == POINT_SMOOTH_IN)
				{
					pointLast->outTan.x = t0;
					pointLast->outTan.y = t1;
				}
				pointLast->distance2Next = t2;
			}
			else
			{
				if ((pointAddr - 1)->flags == POINT_FLAT)
				{
				/* The outTangent of the last point. */
					(pointAddr - 1)->outTan.x = (pointAddr - 1)->outTan.y = 0;
				}
				(pointAddr - 1)->distance2Next = 0;
			}
		}

		if (pathQuery)
		{
			if (numPoints > 0)
			{
				_VGTesstype x, y, w, h;
				w = x = (*points)[0].coord.x;
				h = y = (*points)[0].coord.y;

				for (i = 1; i < numPoints; ++i)
				{
						x = gcmMIN(x, (*points)[i].coord.x);
						y = gcmMIN(y, (*points)[i].coord.y);
						w = gcmMAX(w, (*points)[i].coord.x);
						h = gcmMAX(h, (*points)[i].coord.y);
				}
				path->tessellateResult.bound.x = x;
				path->tessellateResult.bound.y = y;
				path->tessellateResult.bound.width = w - x;
				path->tessellateResult.bound.height = h - y;
			}

			for (i = 0; i < numCommands; ++i)
			{
				segsInfo->items[i].length = 0;
				for (j = 0; j < segsInfo->items[i].numPoints - 1; ++j)
				{
					segsInfo->items[i].length += (*points)[segsInfo->items[i].startPoint + j].distance2Next;
				}
			}
		}
		*numSubPaths -= ppCounter;
	}

#if !USE_FTA
	for (i = 0; i < numSegments; ++i)
    {
        OVG_SAFE_FREE(context->os, pointsInSegment[i]);
    }
    OVG_SAFE_FREE(context->os, pointsInSegment);
    OVG_SAFE_FREE(context->os, numPointsInSegment);
    OVG_SAFE_FREE(context->os, numSegsInSubPath);
#endif

    flattened->numPoints = numPoints;

	gcmFOOTER_ARG("return=%d", numPoints);
    return numPoints;
}

void _ReversePointArray(
    _VGTessPoint points[],
    gctINT32 length,
    gctBOOL        revTangent
    )
{
    _VGTessPoint tempPoint;
    gctINT32 i, j, flag;

	gcmHEADER_ARG("points=0x%x length=%d revTangent=%s",
		points, length, revTangent ? "TRUE" : "FALSE");

    if (length == 1)
	{
		gcmFOOTER_NO();
        return;
	}

    i = 0;
    j = length - 1;

    if (revTangent)
    {
        do
        {
            tempPoint = points[i];
            points[i] = points[j];
            points[j] = tempPoint;
            points[i].inTan.x = -points[i].inTan.x;
            points[i].inTan.y = -points[i].inTan.y;
            points[i].outTan.x = -points[i].outTan.x;
            points[i].outTan.y = -points[i].outTan.y;
            points[j].inTan.x = -points[j].inTan.x;
            points[j].inTan.y = -points[j].inTan.y;
            points[j].outTan.x = -points[j].outTan.x;
            points[j].outTan.y = -points[j].outTan.y;
            ++i;
            --j;
        }while(j > i);
    }
    else
    {
        do
        {
            tempPoint = points[i];
            points[i] = points[j];
            points[j] = tempPoint;
            ++i;
            --j;
        }while(j > i);
    }

    /* Reserve flags. */
    flag = points[0].flags;
    points[0].flags = points[length - 1].flags;
    points[length - 1].flags = (_VGuint8) flag;

	gcmFOOTER_NO();
}

/* End of Filling tessellation. */


/* Stroke Tessellation. ***********************************************/
void    _TessellateFirstLine(
								 _VGContext		*context,
								 _VGTessPoint	linePoints[],
								 gctBOOL		isLastLine,
								 gctBOOL		isClosedPath,
								 _StreamPipe	*streamPipe
    )
{
    _VGTesstype     strokeWidth = context->strokeLineWidth;

	gcmHEADER_ARG("context=0x%x linePoints=0x%x isLastLine=%s isClosedPath=%s streamPipe=0x%x",
		context, linePoints, isLastLine ? "TRUE" : "FALSE", isClosedPath ? "TRUE" : "FALSE", streamPipe);

    if (!isClosedPath)
    {
		_ConstructStartCap(context, linePoints, streamPipe);
    }
    else
    {
		_ConstructStrokeJoin(context, linePoints + 2, linePoints, gcvTRUE, streamPipe);
    }
	_ConstructStrokeBody(context, strokeWidth, linePoints, streamPipe);

    if (isLastLine)
    {
		_ConstructEndCap(context, linePoints, streamPipe);
    }

	gcmFOOTER_NO();
}

void    _TessellateLastLine(
								_VGContext		*context,
								_VGTessPoint	prevLinePoints[],
								_VGTessPoint	currLinePoints[],
								_VGTessPoint	firstLinePoints[],
								_StreamPipe		*streamPipe
    )
{
	_VGVector2		tempVec = {0.0f, 0.0f};

	gcmHEADER_ARG("context=0x%x prevLinePoints=0x%x currrLinePoints=0x%x firstLinePoints=0x%x streamPipe=0x%x",
		context, prevLinePoints, currLinePoints, firstLinePoints, streamPipe);

	_ConstructStrokeJoin(context, prevLinePoints, currLinePoints, gcvFALSE, streamPipe);

    /* For the last line, the out tangent is used to construct the body IF THE PATH IS NOT CLOSED!. */
    if (currLinePoints[1].flags != POINT_FLAT)
    {
        tempVec = currLinePoints[1].inTan;
        currLinePoints[1].inTan = currLinePoints[1].outTan;
    }

	_ConstructStrokeBody(context, context->strokeLineWidth, currLinePoints, streamPipe);

    if (firstLinePoints == gcvNULL)    /* For a un-closed path, add end-cap. */
    {
        _ConstructEndCap(context, currLinePoints, streamPipe);
    }

    /* Restore the in tangent. */
    if (currLinePoints[1].flags != POINT_FLAT)
    {
        currLinePoints[1].inTan = tempVec;
    }

	gcmFOOTER_NO();
}

void    _TessellateMidLine(
							   _VGContext		*context,
							   _VGTessPoint		prevLinePoints[],
							   _VGTessPoint		currLinePoints[],
							   _StreamPipe		*streamPipe
    )
{
	gcmHEADER_ARG("context=0x%x prevLinePoints=0x%x currLinePoints=0x%x streamPipe=0x%x",
		context, prevLinePoints, currLinePoints, streamPipe);

	_ConstructStrokeJoin(context, prevLinePoints,
						 currLinePoints,
						 gcvFALSE,
						 streamPipe
						 );

	_ConstructStrokeBody(context, context->strokeLineWidth,
									  currLinePoints,
									  streamPipe
									  );

	gcmFOOTER_NO();
}

void	_ConstructStartCap(
								_VGContext		*context,
								_VGTessPoint	linePoints[],
								_StreamPipe		*streamPipe
    )
{
    _VGVector2      p0;
    _VGVector2      p1;
    _VGVector2      vLine;
    _VGVector2      vDLine;
    _VGTesstype     capRadius;
    _VGTesstype     tempVar;
    gctINT32        angle0;
    gctINT32        numPoints;
	_VGTessellationContext	*tContext;
     VGCapStyle     cap;
    _VGTesstype     strokeWidth;

	_VGVector2		**stream;
	gctINT32		*currStreamPts;
	gctINT32		*numStreamPts;
	_VGuint16		**indices;
	gctINT32		*currIndex;
	gctINT32		*numIndices;
	_VGVector2		*tempVert;
	_VGuint16		*tempIndx;
	int i;

	gcmHEADER_ARG("context=0x%x linePoints=0x%x streamPipe=0x%x",
		context, linePoints, streamPipe);

	tContext = &context->tessContext;
	cap = context->strokeCapStyle;
	strokeWidth = context->strokeLineWidth;

	stream = &streamPipe->stream;
	currStreamPts = &streamPipe->currStreamPts;
	numStreamPts = &streamPipe->numStreamPts;
	indices = &streamPipe->indices;
	currIndex = &streamPipe->currIndex;
	numIndices = &streamPipe->numIndices;

	if (cap == VG_CAP_BUTT)
    {
		gcmFOOTER_NO();
        return;
    }

	/* Get Vector along the line(vLine) and ortho the line(vDLine) [to the left]. */
    vLine = linePoints[0].outTan;
    vDLine.x = -vLine.y;
    vDLine.y = vLine.x;

    /* Get p0 and p1. */
    capRadius = strokeWidth * 0.5f;
    p0.x = TESS_MUL(capRadius, vDLine.x) + linePoints[0].coord.x;
    p0.y = TESS_MUL(capRadius, vDLine.y) + linePoints[0].coord.y;
    p1.x = TESS_MUL(capRadius, (-vDLine.x)) + linePoints[0].coord.x;
    p1.y = TESS_MUL(capRadius, (-vDLine.y)) + linePoints[0].coord.y;

    switch (cap)
    {
    case VG_CAP_ROUND:
        {
            /* Setup the ellipse for the cap, then tessellate it. */
			angle0 = _GetRadAngle(vDLine.x, vDLine.y) + 1;
			numPoints = _GetCirclePointCount(angle0, angle0 + 178, tContext->strokeJoinStep);
			if( (*currStreamPts + numPoints + 3 > *numStreamPts) ||
				(*currIndex + (numPoints + 1) * 3 > *numIndices))
			{
				_ExpandPipe(context, streamPipe, *currStreamPts + numPoints + 3, *currIndex + (numPoints + 1) * 3);
				stream = &streamPipe->stream;
				indices = &streamPipe->indices;
			}
			tempVert = *stream + *currStreamPts;
			tempIndx = *indices + *currIndex;

			/* Vertex Stream */
			tempVert[0] = linePoints[0].coord;
			_FlattenCircle(&tempVert[0], capRadius, angle0, angle0 + 178, tContext->strokeJoinStep, tempVert + 2);
			tempVert[1] = p0;
			tempVert[numPoints + 2] = p1;
			numPoints++;

			/* Index buffer */
			for (i = 0; i < numPoints; i++)
			{
				tempIndx[i * 3 + 0] = (_VGuint16)(*currStreamPts);
				tempIndx[i * 3 + 1] = (_VGuint16)(*currStreamPts + i + 1);
				tempIndx[i * 3 + 2] = (_VGuint16)(*currStreamPts + i + 2);
			}

			/* Update Counter. */
			*currStreamPts += numPoints + 2;
			*currIndex += numPoints  * 3;
        }
        break;

    case VG_CAP_SQUARE:
        {
			if( (*currStreamPts + 5 > *numStreamPts) ||
				(*currIndex + 9 > *numIndices))
			{
				_ExpandPipe(context, streamPipe, *currStreamPts + 5, *currIndex + 9);
				stream = &streamPipe->stream;
				indices = &streamPipe->indices;
			}

			tempVert = *stream + *currStreamPts;
			tempIndx = *indices + *currIndex;

			/*
				0--2
				| >3
				1--4	-->line direction
			*/
			/* Vertex stream. */
			tempVert[2] = p0;
			tempVert[3] = linePoints[0].coord;
			tempVert[4] = p1;
            tempVar = TESS_MUL(capRadius, vLine.x);
			tempVert[0].x = p0.x - tempVar;
			tempVert[1].x = p1.x - tempVar;
            tempVar = TESS_MUL(capRadius, vLine.y);
			tempVert[0].y = p0.y - tempVar;
			tempVert[1].y = p1.y - tempVar;

			/* Index buffer. */
			tempIndx[0] = (_VGuint16)(*currStreamPts + 3);
			tempIndx[1] = (_VGuint16)(*currStreamPts + 2);
			tempIndx[2] = (_VGuint16)(*currStreamPts);
			tempIndx[3] = (_VGuint16)(*currStreamPts + 3);
			tempIndx[4] = (_VGuint16)(*currStreamPts);
			tempIndx[5] = (_VGuint16)(*currStreamPts + 1);
			tempIndx[6] = (_VGuint16)(*currStreamPts + 3);
			tempIndx[7] = (_VGuint16)(*currStreamPts + 1);
			tempIndx[8] = (_VGuint16)(*currStreamPts + 4);

			/* Update counter. */
			*currStreamPts += 5;
			*currIndex += 9;
        }
        break;

    default:
        break;
    }

	gcmFOOTER_NO();
}

void    _ConstructStrokeBody(
							 _VGContext		*context,
							 _VGTesstype	strokeWidth,
							 _VGTessPoint	linePoints[],
							 _StreamPipe	*streamPipe
    )
{
    _VGVector2  vLine0, vLine1;
    _VGTesstype radius;

	gctINT32	sizeVert, sizeIndx;
	_VGVector2	*tempVert;
	_VGuint16	*tempIndx;

	gcmHEADER_ARG("context=0x%x strokeWidth=%f linePoints=0x%x streamPipe=0x%x",
		context, strokeWidth, linePoints, streamPipe);

    vLine0 = linePoints[0].outTan;
	vLine1 = linePoints[1].inTan;
    radius = TESS_MUL(strokeWidth, TESS_CONST_HALF);

	sizeVert = streamPipe->currStreamPts + 6;
	sizeIndx = streamPipe->currIndex + 12;
	if ((sizeVert > streamPipe->numStreamPts) ||
		(sizeIndx > streamPipe->numIndices))
	{
		_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
	}
	/* Layout
	0-------3
	1-------4	-->line direction
	2-------5
	*/
	tempVert = streamPipe->stream + streamPipe->currStreamPts;
	tempIndx = streamPipe->indices + streamPipe->currIndex;

	tempVert[0].x = TESS_MUL(radius, (-vLine0.y)) + linePoints[0].coord.x;
	tempVert[0].y = TESS_MUL(radius, vLine0.x) + linePoints[0].coord.y;
	tempVert[1]   = linePoints[0].coord;
	tempVert[2].x = tempVert[1].x + tempVert[1].x - tempVert[0].x;
    tempVert[2].y = tempVert[1].y + tempVert[1].y - tempVert[0].y;

	tempVert[3].x = TESS_MUL(radius, (-vLine1.y)) + linePoints[1].coord.x;
	tempVert[3].y = TESS_MUL(radius, vLine1.x) + linePoints[1].coord.y;
	tempVert[4]   = linePoints[1].coord;
	tempVert[5].x = tempVert[4].x + tempVert[4].x - tempVert[3].x;
    tempVert[5].y = tempVert[4].y + tempVert[4].y - tempVert[3].y;

	tempIndx[11]= (_VGuint16)streamPipe->currStreamPts;
	tempIndx[0] = tempIndx[3] = tempIndx[6] = tempIndx[9] = tempIndx[11] + 1;
	tempIndx[1] = tempIndx[11] + 2;
	tempIndx[2] = tempIndx[4] = tempIndx[11] + 5;
	tempIndx[5] = tempIndx[7] = tempIndx[11] + 4;
	tempIndx[8] = tempIndx[10]= tempIndx[11] + 3;

	streamPipe->currStreamPts = sizeVert;
	streamPipe->currIndex = sizeIndx;

	gcmFOOTER_NO();
}

void    _ConstructEndCap(
						 _VGContext		*context,
						 _VGTessPoint	linePoints[],
						 _StreamPipe	*streamPipe
    )
{
    _VGVector2      p0;
    _VGVector2      p1;
    _VGVector2      vLine;
    _VGVector2		vDLine;
    _VGTesstype     capRadius;
    _VGTesstype     tempVar;
    gctINT32	    angle0;
    gctINT32        numPoints;
     VGCapStyle		cap;

	_VGVector2		**stream;
	gctINT32		*currStreamPts;
	gctINT32		*numStreamPts;
	_VGuint16		**indices;
	gctINT32		*currIndex;
	gctINT32		*numIndices;
	_VGVector2		*tempVert;
	_VGuint16		*tempIndx;
	int i;

	gcmHEADER_ARG("context=0x%x linePoints=0x%x streamPipe=0x%x", context, linePoints, streamPipe);

	cap = context->strokeCapStyle;
    if (cap == VG_CAP_BUTT)
    {
		gcmFOOTER_NO();
        return;
    }

	stream = &streamPipe->stream;
	currStreamPts = &streamPipe->currStreamPts;
	numStreamPts = &streamPipe->numStreamPts;
	indices = &streamPipe->indices;
	currIndex = &streamPipe->currIndex;
	numIndices = &streamPipe->numIndices;

    /* Get Vector along the line(vLine) and ortho the line(vDLine) [to the left]. */
	vLine = linePoints[1].inTan;
    vDLine.x = -vLine.y;
    vDLine.y = vLine.x;

    /* Get p0 and p1. */
    capRadius = TESS_MUL(context->strokeLineWidth, TESS_CONST_HALF);
    p0.x = TESS_MUL(capRadius, (-vDLine.x)) + linePoints[1].coord.x;
    p0.y = TESS_MUL(capRadius, (-vDLine.y)) + linePoints[1].coord.y;
    p1.x = TESS_MUL(capRadius, vDLine.x) + linePoints[1].coord.x;
    p1.y = TESS_MUL(capRadius, vDLine.y) + linePoints[1].coord.y;

    switch (cap)
    {
    case VG_CAP_ROUND:
        {
			angle0 = _GetRadAngle(-vDLine.x, -vDLine.y) + 1;
			numPoints = _GetCirclePointCount(angle0, angle0 + 178, context->tessContext.strokeJoinStep);
			if ((*currStreamPts + numPoints + 3 > *numStreamPts) ||
				(*currIndex + (numPoints + 1) * 3 > *numIndices))
			{
				_ExpandPipe(context, streamPipe, *currStreamPts + numPoints + 3, *currIndex + (numPoints + 1) * 3);
				stream = &streamPipe->stream;
				indices = &streamPipe->indices;
			}
			tempVert = *stream + *currStreamPts;
			tempIndx = *indices + *currIndex;

			tempVert[0] = linePoints[1].coord;
			tempVert[1] = p0;
			_FlattenCircle(&tempVert[0], capRadius, angle0, angle0 + 178, context->tessContext.strokeJoinStep, tempVert + 2);
			tempVert[numPoints + 2] = p1;
			numPoints++;

			for (i = 0; i < numPoints; i++)
			{
				tempIndx[i * 3 + 0] = (_VGuint16)(*currStreamPts);
				tempIndx[i * 3 + 1] = (_VGuint16)(*currStreamPts + i + 1);
				tempIndx[i * 3 + 2] = (_VGuint16)(*currStreamPts + i + 2);
			}

			*currStreamPts += numPoints + 2;
			*currIndex += numPoints * 3;
        }
        break;

    case VG_CAP_SQUARE:
        {
			if( (*currStreamPts + 5 > *numStreamPts)||
				(*currIndex + 9 > *numIndices))
			{
				_ExpandPipe(context, streamPipe, *currStreamPts + 5, *currIndex + 9);
				stream = &streamPipe->stream;
				indices = &streamPipe->indices;
			}
			tempVert = *stream + *currStreamPts;
			tempIndx = *indices + *currIndex;

			/*
			Vertex Layout. line direction --> right
			 0--3
			 1< |
			 2--4
			*/
			tempVert[0] = p1;
			tempVert[1] = linePoints[1].coord;
			tempVert[2] = p0;
            tempVar = TESS_MUL(capRadius, vLine.x);
            tempVert[3].x = p1.x + tempVar;
            tempVert[4].x = p0.x + tempVar;
            tempVar = TESS_MUL(capRadius, vLine.y);
            tempVert[3].y = p1.y + tempVar;
            tempVert[4].y = p0.y + tempVar;

			tempIndx[0] = (_VGuint16)(*currStreamPts + 1);
			tempIndx[1] = (_VGuint16)(*currStreamPts + 3);
			tempIndx[2] = (_VGuint16)(*currStreamPts);

			tempIndx[3] = (_VGuint16)(*currStreamPts + 1);
			tempIndx[4] = (_VGuint16)(*currStreamPts + 4);
			tempIndx[5] = (_VGuint16)(*currStreamPts + 3);

			tempIndx[6] = (_VGuint16)(*currStreamPts + 1);
			tempIndx[7] = (_VGuint16)(*currStreamPts + 2);
			tempIndx[8] = (_VGuint16)(*currStreamPts + 4);

			*currStreamPts += 5;
			*currIndex += 9;
        }
        break;

    default:
        gcmASSERT(0);
        break;
    }

	gcmFOOTER_NO();
}

void    _ConstructStrokeJoin(
							 _VGContext		*context,
							 _VGTessPoint   prevLinePoints[],
							 _VGTessPoint   currLinePoints[],
							 gctBOOL        closeJoin,
							 _StreamPipe	*streamPipe
    )
{
    gctINT32		numPoints;
    _VGVector2      vLine0, vLine1;
    _VGVector2		vDLine0, vDLine1;
    _VGTesstype     prod, radius;
    _VGVector2      p0, p1, p2, p3;

     VGJoinStyle	join;
    _VGTesstype		strokeWidth;

	_VGVector2		*tempVert;
	_VGuint16		*tempIndx;
	gctINT32		sizeVert, sizeIndx;
	gctINT32		step;
	gctINT32	    angle0, angle1;
	gctINT32		i;
	gctFLOAT		tempX, tempY;

	gcmHEADER_ARG("context=0x%x prevLinePoints=0x%x currLinePoints=0x%x closeJoin=%s streamPipe=0x%x",
		context, prevLinePoints, currLinePoints, closeJoin ? "TRUE" : "FALSE", streamPipe);

	join = context->strokeJoinStyle;
	strokeWidth = context->strokeLineWidth;

    vLine0 = prevLinePoints[1].inTan;
    vLine1 = currLinePoints[0].outTan;
    vDLine0.x = -vLine0.y;
    vDLine0.y = vLine0.x;
    vDLine1.x = -vLine1.y;
    vDLine1.y = vLine1.x;
    prod = TESS_MUL(vLine0.x, vLine1.y) - TESS_MUL(vLine0.y, vLine1.x);
    radius = TESS_MUL(strokeWidth, TESS_CONST_HALF);
    if (TESS_ABS(prod) < ZEROERROR)
        prod = 0;
    if (prod == 0)
    {
        if(((_VGTessHighType)vLine0.x * vLine1.x > 0) ||
           ((_VGTessHighType)vLine0.y * vLine1.y > 0))
        {
			gcmFOOTER_NO();
            return;
        }
        else
        {
            if ((join != VG_JOIN_ROUND) &&
                ((currLinePoints[0].flags & POINT_SMOOTH) == 0))
            {
				gcmFOOTER_NO();
                return;
            }
			else
			{
				/* Two half circles make up the join. */
				p0.x = currLinePoints[0].coord.x + TESS_MUL(radius, vDLine1.x);
				p0.y = currLinePoints[0].coord.y + TESS_MUL(radius, vDLine1.y);

				/*First, construct the join geometry on the left side. */
				p2.x = currLinePoints[0].coord.x - TESS_MUL(radius, vLine1.x);
				p2.y = currLinePoints[0].coord.y - TESS_MUL(radius, vLine1.y);
				p1.x = currLinePoints[0].coord.x - TESS_MUL(radius, vDLine1.x);
				p1.y = currLinePoints[0].coord.y - TESS_MUL(radius, vDLine1.y);
				angle0 = _GetRadAngle(vDLine1.x, vDLine1.y) + 1;

				numPoints = _GetCirclePointCount(angle0, angle0 + 178, context->tessContext.strokeJoinStep);
				sizeVert = streamPipe->currStreamPts + numPoints * 2 + 2;
				sizeIndx = streamPipe->currIndex + numPoints * 6 + 6;
				if ((sizeVert > streamPipe->numStreamPts) ||
					(sizeIndx > streamPipe->numIndices))
				{
					_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
				}
				tempVert = streamPipe->stream + streamPipe->currStreamPts;
				tempIndx = streamPipe->indices + streamPipe->currIndex;

				tempVert[0] = currLinePoints[0].coord;
				tempVert[1] = p0;
				_FlattenCircle(&tempVert[0], radius, angle0, angle0 + 178, context->tessContext.strokeJoinStep, tempVert + 2);
				tempVert[numPoints + 2] = p1;
				numPoints += 2;

				for (i = 1; i < numPoints ; i++)
				{
					tempVert[i + numPoints - 1].x = currLinePoints[0].coord.x * 2.0f - tempVert[i].x;
					tempVert[i + numPoints - 1].y = currLinePoints[0].coord.y * 2.0f - tempVert[i].y;
				}

				numPoints = (numPoints - 1) * 2 - 1;
				for (i = 0; i < numPoints; i++)
				{
					*tempIndx++ = (_VGuint16)streamPipe->currStreamPts;
					*tempIndx++ = (_VGuint16)(streamPipe->currStreamPts + i + 1);
					*tempIndx++ = (_VGuint16)(streamPipe->currStreamPts + i + 2);
				}

				tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
				tempIndx[1] = (_VGuint16)(streamPipe->currStreamPts + i + 1);
				tempIndx[2] = (_VGuint16)(streamPipe->currStreamPts + 1);

				streamPipe->currStreamPts = sizeVert;
				streamPipe->currIndex = sizeIndx;

				gcmFOOTER_NO();
				return;
			}
        }
    }

	if ((prevLinePoints + 1 == currLinePoints) &&
        (currLinePoints[0].flags == POINT_SMOOTH) &&
        !closeJoin)
    {
        p0.x = currLinePoints[0].coord.x + TESS_MUL(radius, vDLine1.x);
        p0.y = currLinePoints[0].coord.y + TESS_MUL(radius, vDLine1.y);

		/*First, construct the join geometry on the left side. */
        p1.x = currLinePoints[0].coord.x + TESS_MUL(radius, vDLine0.x);
        p1.y = currLinePoints[0].coord.y + TESS_MUL(radius, vDLine0.y);
        p2.x = currLinePoints[0].coord.x;
        p2.y = currLinePoints[0].coord.y;

		{
			/* 1st side. */
        if (prod > 0)   /* If left turn, the points array should be reverted. */
        {
			angle0 = _GetRadAngle(vDLine0.x, vDLine0.y);
			angle1 = _GetRadAngle(vDLine1.x, vDLine1.y);
        }
		else
		{
			angle0 = _GetRadAngle(vDLine1.x, vDLine1.y);
			angle1 = _GetRadAngle(vDLine0.x, vDLine0.y);
		}

		if (angle0 > angle1)
		{
			angle1 += 360;
		}
		if ((1.0f - CIRCLE_COS[(angle1 - angle0) / 2]) * radius * context->tessContext.strokeScale > 1.0f)
		{
			angle0++;
			angle1--;
			numPoints = _GetCirclePointCount(angle0, angle1, context->tessContext.strokeJoinStep);

			sizeVert = streamPipe->currStreamPts + numPoints + numPoints;
			sizeIndx = streamPipe->currIndex + numPoints * 6 + 6;
			if ((sizeVert > streamPipe->numStreamPts) ||
				(sizeIndx > streamPipe->numIndices))
			{
				_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
			}
			tempVert = streamPipe->stream + streamPipe->currStreamPts;
			tempIndx = streamPipe->indices + streamPipe->currIndex;

			_FlattenCircle(&currLinePoints[0].coord, radius, angle0, angle1, context->tessContext.strokeJoinStep, tempVert);
			tempX = currLinePoints[0].coord.x + currLinePoints[0].coord.x;
			tempY = currLinePoints[0].coord.y + currLinePoints[0].coord.y;
			for (i = 0; i < numPoints; i++)
			{
				tempVert[numPoints].x = tempX - tempVert->x;
				tempVert[numPoints].y = tempY - tempVert->y;
				tempVert++;
			}

			tempIndx[0] = tempIndx[3] = tempIndx[6] = tempIndx[9] = (_VGuint16)(streamPipe->currStreamPts - 2);
			if (prod > 0)	/* Left turn */
			{
				/* The four triangles on the edges. */
				tempIndx[1] = (_VGuint16)(streamPipe->currStreamPts - 3);
				tempIndx[2] = (_VGuint16)(streamPipe->currStreamPts);
				tempIndx[4] = (_VGuint16)(streamPipe->currStreamPts + numPoints - 1);
				tempIndx[5] = (_VGuint16)(streamPipe->currStreamPts + numPoints + numPoints);
				tempIndx[7] = (_VGuint16)(streamPipe->currStreamPts - 1);
				tempIndx[8] = (_VGuint16)(streamPipe->currStreamPts + numPoints);
				tempIndx[10] = (_VGuint16)(streamPipe->currStreamPts + numPoints + numPoints - 1);
				tempIndx[11] = (_VGuint16)(streamPipe->currStreamPts + numPoints + numPoints + 2);
			}
			else
			{
				/* The four triangles on the edges. */
				tempIndx[1] = (_VGuint16)(streamPipe->currStreamPts + numPoints + numPoints);
				tempIndx[2] = (_VGuint16)(streamPipe->currStreamPts);
				tempIndx[4] = (_VGuint16)(streamPipe->currStreamPts + numPoints - 1);
				tempIndx[5] = (_VGuint16)(streamPipe->currStreamPts - 3);
				tempIndx[7] = (_VGuint16)(streamPipe->currStreamPts + numPoints + numPoints + 2);
				tempIndx[8] = (_VGuint16)(streamPipe->currStreamPts + numPoints);
				tempIndx[10] = (_VGuint16)(streamPipe->currStreamPts + numPoints + numPoints - 1);
				tempIndx[11] = (_VGuint16)(streamPipe->currStreamPts - 1);
			}
			tempIndx += 12;

			step = numPoints * 3 - 3;
			for (i = 0; i < numPoints - 1; i++)
			{
				*(tempIndx + step) = *tempIndx = (_VGuint16)(streamPipe->currStreamPts - 2);
				tempIndx++;
				*(tempIndx + step) = (_VGuint16)(streamPipe->currStreamPts + numPoints + i);
				*tempIndx++ = (_VGuint16)(streamPipe->currStreamPts + i);
				*(tempIndx + step) = (_VGuint16)(streamPipe->currStreamPts + numPoints + i + 1);
				*tempIndx++ = (_VGuint16)(streamPipe->currStreamPts + i + 1);
			}

			streamPipe->currStreamPts = sizeVert;
			streamPipe->currIndex = sizeIndx;
		}
        else
        {
			sizeVert = streamPipe->currStreamPts + 5;
			sizeIndx = streamPipe->currIndex + 6;
			if ((sizeVert > streamPipe->numStreamPts) ||
				(sizeIndx > streamPipe->numIndices))
			{
				_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
			}
			tempVert = streamPipe->stream + streamPipe->currStreamPts;
			tempIndx = streamPipe->indices + streamPipe->currIndex;

			tempVert[0] = p2;
			tempVert[1] = p0;
			tempVert[2] = p1;
			tempVert[3].x = p2.x + p2.x - p0.x;
			tempVert[3].y = p2.y + p2.y - p0.y;
			tempVert[4].x = p2.x + p2.x - p1.x;
			tempVert[4].y = p2.y + p2.y - p1.y;

			tempIndx[0] = tempIndx[3] = (_VGuint16)streamPipe->currStreamPts;
			tempIndx[1] = tempIndx[0] + 1;
			tempIndx[2] = tempIndx[0] + 2;
			tempIndx[4] = tempIndx[0] + 4;
			tempIndx[5] = tempIndx[0] + 3;

			streamPipe->currStreamPts = sizeVert;
			streamPipe->currIndex = sizeIndx;
        }}
    }
	else
    switch (join)
    {
		case VG_JOIN_ROUND:
        {
            p2.x = currLinePoints[0].coord.x;
            p2.y = currLinePoints[0].coord.y;

            if (prod < 0)
            {
				/*First, construct the join geometry on the left side. */
				p0.x = currLinePoints[0].coord.x + TESS_MUL(radius, vDLine1.x);
				p0.y = currLinePoints[0].coord.y + TESS_MUL(radius, vDLine1.y);
				p1.x = currLinePoints[0].coord.x + TESS_MUL(radius, vDLine0.x);
				p1.y = currLinePoints[0].coord.y + TESS_MUL(radius, vDLine0.y);
				angle0 = _GetRadAngle(vDLine1.x, vDLine1.y);
				angle1 = _GetRadAngle(vDLine0.x, vDLine0.y);

				if (angle0 > angle1)
				{
					angle1 += 360;
				}
				if ((1.0f - CIRCLE_COS[(angle1 - angle0) / 2]) * radius * context->tessContext.strokeScale > 1.0f)
				{
					angle0++;
					angle1--;
					numPoints = _GetCirclePointCount(angle0, angle1, context->tessContext.strokeJoinStep);
					sizeVert = streamPipe->currStreamPts + numPoints + 3;
					sizeIndx = streamPipe->currIndex + numPoints * 3 + 3;
					if ((sizeVert > streamPipe->numStreamPts) ||
						(sizeIndx > streamPipe->numIndices))
					{
						_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
					}
					tempVert = streamPipe->stream + streamPipe->currStreamPts;
					tempIndx = streamPipe->indices + streamPipe->currIndex;

					tempVert[0] = currLinePoints[0].coord;
					tempVert[1] = p0;
					numPoints += 2;
					_FlattenCircle(&tempVert[0], radius, angle0, angle1, context->tessContext.strokeJoinStep, tempVert + 2);
					tempVert[numPoints] = p1;

					for (i = 1; i < numPoints; i++)
					{
						tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
						tempIndx[1] = (_VGuint16) (tempIndx[0] + i);
						tempIndx[2] = (_VGuint16) (tempIndx[1] + 1);
						tempIndx += 3;
					}

					streamPipe->currStreamPts = sizeVert;
					streamPipe->currIndex = sizeIndx;

				}
                else
                {
					sizeVert = streamPipe->currStreamPts + 3;
					sizeIndx = streamPipe->currIndex + 3;
					if ((sizeVert > streamPipe->numStreamPts) ||
						(sizeIndx > streamPipe->numIndices))
					{
						_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
					}
					tempVert = streamPipe->stream + streamPipe->currStreamPts;
					tempIndx = streamPipe->indices + streamPipe->currIndex;

					tempVert[0] = p2;
					tempVert[1] = p0;
					tempVert[2] = p1;

					tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
					tempIndx[1] = tempIndx[0] + 1;
					tempIndx[2] = tempIndx[0] + 2;

					streamPipe->currStreamPts = sizeVert;
					streamPipe->currIndex = sizeIndx;
                }
			}
            else
            {
				/*Then construct the join geometry on the right side. */
				p0.x = currLinePoints[0].coord.x - TESS_MUL(radius, vDLine0.x);
				p0.y = currLinePoints[0].coord.y - TESS_MUL(radius, vDLine0.y);
				p1.x = currLinePoints[0].coord.x - TESS_MUL(radius, vDLine1.x);
				p1.y = currLinePoints[0].coord.y - TESS_MUL(radius, vDLine1.y);
				angle0 = _GetRadAngle(-vDLine0.x, -vDLine0.y);
				angle1 = _GetRadAngle(-vDLine1.x, -vDLine1.y);
				if (angle0 > angle1)
				{
					angle1 += 360;
				}
				if ((1.0f - CIRCLE_COS[(angle1 - angle0) / 2]) * radius * context->tessContext.strokeScale > 1.0f)
				{
					angle0++;
					angle1--;
					numPoints = _GetCirclePointCount(angle0, angle1, context->tessContext.strokeJoinStep);
					sizeVert = streamPipe->currStreamPts + numPoints + 3;
					sizeIndx = streamPipe->currIndex + numPoints * 3 + 3;
					if ((sizeVert > streamPipe->numStreamPts) ||
						(sizeIndx > streamPipe->numIndices))
					{
						_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
					}
					tempVert = streamPipe->stream + streamPipe->currStreamPts;
					tempIndx = streamPipe->indices + streamPipe->currIndex;

					tempVert[0] = currLinePoints[0].coord;
					tempVert[1] = p0;
					numPoints += 2;
					_FlattenCircle(&tempVert[0], radius, angle0, angle1, context->tessContext.strokeJoinStep, tempVert + 2);
					tempVert[numPoints] = p1;

					for (i = 1; i < numPoints; i++)
					{
						tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
						tempIndx[1] = (_VGuint16) (tempIndx[0] + i);
						tempIndx[2] = (_VGuint16) (tempIndx[1] + 1);
						tempIndx += 3;
					}

					streamPipe->currStreamPts = sizeVert;
					streamPipe->currIndex = sizeIndx;
                }
                else
                {
					sizeVert = streamPipe->currStreamPts + 3;
					sizeIndx = streamPipe->currIndex + 3;
					if ((sizeVert > streamPipe->numStreamPts) ||
						(sizeIndx > streamPipe->numIndices))
					{
						_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
					}
					tempVert = streamPipe->stream + streamPipe->currStreamPts;
					tempIndx = streamPipe->indices + streamPipe->currIndex;

					tempVert[0] = p2;
					tempVert[1] = p1;
					tempVert[2] = p0;

					tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
					tempIndx[1] = tempIndx[0] + 1;
					tempIndx[2] = tempIndx[0] + 2;

					streamPipe->currStreamPts = sizeVert;
					streamPipe->currIndex = sizeIndx;
                }
            }
        }
        break;

		case VG_JOIN_MITER:
        {
            _VGTesstype     tanHalfA, length;
            _VGTesstype     miterSize;
			_VGTesstype		temp, t;

            /* This is the sin of the two lines. */
			t = TESS_CONST_ONE - TESS_MUL(prod, prod);
			temp  = gcmMAX(t, 0);
            if (TESS_MUL(vDLine0.x, vLine1.y) - TESS_MUL(vDLine0.y, vLine1.x) > 0)
            {
                tanHalfA = TESS_ABS(TESS_DIV(TESS_CONST_ONE + TESS_SQRT(temp), prod)); /* avoid 'div by zero' */
            }
            else
            {
                tanHalfA = TESS_ABS(TESS_DIV(prod, (TESS_CONST_ONE + TESS_SQRT(temp))));
            }
            length = TESS_MUL(radius, tanHalfA);

            miterSize = TESS_MUL(TESSTYPE(context->strokeMiterLimit), strokeWidth);
            if (TESS_MUL(miterSize, miterSize) < TESS_MUL(TESS_MUL(length, length), TESS_CONST_4) + TESS_MUL(strokeWidth, strokeWidth))
            {
				VGJoinStyle style = context->strokeJoinStyle;
				context->strokeJoinStyle = VG_JOIN_BEVEL;
				_ConstructStrokeJoin(context, prevLinePoints,
									 currLinePoints,
									 closeJoin,
									 streamPipe
									 );
				context->strokeJoinStyle = style;
                break;
            }

            p0.x = currLinePoints[0].coord.x + TESS_MUL(radius, vDLine1.x);
            p0.y = currLinePoints[0].coord.y + TESS_MUL(radius, vDLine1.y);
            numPoints = 4;
            if (prod < 0)    /* Right turn */
            {
                p1.x = p0.x - TESS_MUL(length, vLine1.x);
                p1.y = p0.y - TESS_MUL(length, vLine1.y);
                p2.x = prevLinePoints[1].coord.x + TESS_MUL(radius, vDLine0.x);
                p2.y = prevLinePoints[1].coord.y + TESS_MUL(radius, vDLine0.y);
                p3.x = currLinePoints[0].coord.x;
                p3.y = currLinePoints[0].coord.y;

				{
					sizeVert = streamPipe->currStreamPts + 4;
					sizeIndx = streamPipe->currIndex + 6;
					if ((sizeVert > streamPipe->numStreamPts) ||
						(sizeIndx > streamPipe->numIndices))
					{
						_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
					}
					tempVert = streamPipe->stream + streamPipe->currStreamPts;
					tempIndx = streamPipe->indices + streamPipe->currIndex;

					tempVert[0] = p3;
					tempVert[1] = p0;
					tempVert[2] = p1;
					tempVert[3] = p2;

					tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
					tempIndx[1] = (_VGuint16)(streamPipe->currStreamPts + 1);
					tempIndx[2] = (_VGuint16)(streamPipe->currStreamPts + 2);
					tempIndx[3] = (_VGuint16)streamPipe->currStreamPts;
					tempIndx[4] = (_VGuint16)(streamPipe->currStreamPts + 2);
					tempIndx[5] = (_VGuint16)(streamPipe->currStreamPts + 3);

					streamPipe->currStreamPts = sizeVert;
					streamPipe->currIndex = sizeIndx;
				}
            }
            else                /* Left turn */
            {
                p0 = currLinePoints[0].coord;
                p1.x = prevLinePoints[1].coord.x - TESS_MUL(radius, vDLine0.x);
                p1.y = prevLinePoints[1].coord.y - TESS_MUL(radius, vDLine0.y);
                p2.x = p1.x + TESS_MUL(length, vLine0.x);
                p2.y = p1.y + TESS_MUL(length, vLine0.y);
                p3.x = currLinePoints[0].coord.x - TESS_MUL(radius, vDLine1.x);
                p3.y = currLinePoints[0].coord.y - TESS_MUL(radius, vDLine1.y);

				{
					sizeVert = streamPipe->currStreamPts + 4;
					sizeIndx = streamPipe->currIndex + 6;
					if ((sizeVert > streamPipe->numStreamPts) ||
						(sizeIndx > streamPipe->numIndices))
					{
						_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
					}
					tempVert = streamPipe->stream + streamPipe->currStreamPts;
					tempIndx = streamPipe->indices + streamPipe->currIndex;

					tempVert[0] = p0;
					tempVert[1] = p1;
					tempVert[2] = p2;
					tempVert[3] = p3;

					tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
					tempIndx[1] = (_VGuint16)(streamPipe->currStreamPts + 1);
					tempIndx[2] = (_VGuint16)(streamPipe->currStreamPts + 2);
					tempIndx[3] = (_VGuint16)streamPipe->currStreamPts;
					tempIndx[4] = (_VGuint16)(streamPipe->currStreamPts + 2);
					tempIndx[5] = (_VGuint16)(streamPipe->currStreamPts + 3);

					streamPipe->currStreamPts = sizeVert;
					streamPipe->currIndex = sizeIndx;
				}
            }
        }
        break;

		case VG_JOIN_BEVEL:
        {
            numPoints = 3;

            if (prod < 0)      /* right */
            {
				sizeVert = streamPipe->currStreamPts + 3;
				sizeIndx = streamPipe->currIndex + 3;
				if ((sizeVert > streamPipe->numStreamPts) ||
					(sizeIndx > streamPipe->numIndices))
				{
					_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
				}
				tempVert = streamPipe->stream + streamPipe->currStreamPts;
				tempIndx = streamPipe->indices + streamPipe->currIndex;

				tempVert[0] = currLinePoints[0].coord;
				tempVert[1].x = currLinePoints[0].coord.x + radius * vDLine1.x;
				tempVert[1].y = currLinePoints[0].coord.y + radius * vDLine1.y;
				tempVert[2].x = currLinePoints[0].coord.x + radius * vDLine0.x;
				tempVert[2].y = currLinePoints[0].coord.y + radius * vDLine0.y;

				tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
				tempIndx[1] = (_VGuint16)streamPipe->currStreamPts + 1;
				tempIndx[2] = (_VGuint16)streamPipe->currStreamPts + 2;

				streamPipe->currStreamPts = sizeVert;
				streamPipe->currIndex = sizeIndx;
            }
            else
            {
				sizeVert = streamPipe->currStreamPts + 3;
				sizeIndx = streamPipe->currIndex + 3;
				if ((sizeVert > streamPipe->numStreamPts) ||
					(sizeIndx > streamPipe->numIndices))
				{
					_ExpandPipe(context, streamPipe, sizeVert, sizeIndx);
				}
				tempVert = streamPipe->stream + streamPipe->currStreamPts;
				tempIndx = streamPipe->indices + streamPipe->currIndex;

				tempVert[0] = currLinePoints[0].coord;
				tempVert[1].x = currLinePoints[0].coord.x - radius * vDLine0.x;
				tempVert[1].y = currLinePoints[0].coord.y - radius * vDLine0.y;
				tempVert[2].x = currLinePoints[0].coord.x - radius * vDLine1.x;
				tempVert[2].y = currLinePoints[0].coord.y - radius * vDLine1.y;

				tempIndx[0] = (_VGuint16)streamPipe->currStreamPts;
				tempIndx[1] = (_VGuint16)(streamPipe->currStreamPts + 1);
				tempIndx[2] = (_VGuint16)(streamPipe->currStreamPts + 2);

				streamPipe->currStreamPts = sizeVert;
				streamPipe->currIndex = sizeIndx;
            }
        }
        break;

		default:
        gcmASSERT(0);
        break;
    }

	gcmFOOTER_NO();
}

void	_ConstructStroke(
							 _VGContext		*context,
							 _VGTessPoint	*points,
							 gctINT32		numPoints,
							 gctBOOL		pathClosed,
							 _StreamPipe	*streamPipe
    )
{
    gctINT32        i, j;
    gctINT32        numLines;
    _VGTessPoint    *tempPoints;
    _VGTessPoint    *pointsCopy;
    gctBOOL         dupFound = gcvFALSE;
    _VGTesstype		radius;
    _VGTesstype     width;
    VGCapStyle      cap;

	_VGVector2		**stream;
	gctINT32		*currStreamPts;
	gctINT32		*numStreamPts;
	_VGuint16		**indices;
	gctINT32		*currIndex;
	gctINT32		*numIndices;

	gcmHEADER_ARG("context=0x%x points=0x%x numPoints=%d pathClosed=%s streamPipe=0x%x",
				  context, points, numPoints, pathClosed ? "TRUE" : "FALSE", streamPipe);

	width = context->strokeLineWidth;
	cap = context->strokeCapStyle;

	stream	= &streamPipe->stream;
	currStreamPts = &streamPipe->currStreamPts;
	numStreamPts = &streamPipe->numStreamPts;
	indices = &streamPipe->indices;
	currIndex = &streamPipe->currIndex;
	numIndices = &streamPipe->numIndices;

	OVG_MALLOC(context->os, pointsCopy, sizeof(_VGTessPoint) * numPoints);
    j = 0;
    /* Degenerated lines also need to be drawn, so the following lines are disabled.?? */
    if (numPoints == 1)
    {
        pointsCopy[0] = points[0];
    }
    else
    {
        for (i = 0; i < numPoints - 1; ++i)
        {
            /* Get rid of the degenerated lines. */
            if ((points[i].coord.x == points[i + 1].coord.x) &&
                (points[i].coord.y == points[i + 1].coord.y))
            {
                if (!dupFound)
                {
                    pointsCopy[j] = points[i];
                    dupFound = gcvTRUE;
                }
                continue;
            }
            else
            {
                if (dupFound)
                {
                    pointsCopy[j].outTan = points[i].outTan;    /* no distance to next is needed. */
                    if ((pointsCopy[j].flags != POINT_SMOOTH) &&
                        (points[i].flags == POINT_SMOOTH))
                    {
                        points[i].flags = POINT_SMOOTH_OUT;
                    }
                    pointsCopy[j].flags |= points[i].flags;
                }
                else
                {
                    pointsCopy[j] = points[i];
                }
                j++;
                dupFound = gcvFALSE;
            }
        }
        if (j == 0)
        {
            pointsCopy[j].inTan = pointsCopy[j].outTan = points[i].inTan;
            j++;
        }else
        if (!VECTOR_EQUAL(pointsCopy[j - 1].coord, points[i].coord))
        {
            pointsCopy[j++] = points[i];
        }
        else
        {
            pointsCopy[j++].outTan = points[i].outTan;
        }

        numPoints = j;
    }
    points = pointsCopy;

	radius = TESS_MUL(width, TESS_CONST_HALF);
    if (numPoints < 2)
    {
        if (numPoints == 1)
        {   /* According to the spec 1.1 p 98, 0 sized dash should be drawn if the cap type is ROUND or SQUARE. */
            if (cap == VG_CAP_ROUND)
            {
                /* Handle zero dash length here. */
				_VGVector2		*tempVert;
				_VGuint16		*tempIndx;
                _VGVector2		tp0;

				tp0.x = points[0].coord.x + radius;
                tp0.y = points[0].coord.y;
				numPoints = _GetCirclePointCount(1, 359, context->tessContext.strokeJoinStep);
				if ((*currStreamPts + numPoints + 1 > *numStreamPts) ||
					(*currIndex + numPoints * 3 > *numIndices))
				{
					_ExpandPipe(context, streamPipe, *currStreamPts + numPoints + 1, *currIndex + numPoints * 3);
					stream = &streamPipe->stream;
					indices = &streamPipe->indices;
				}
				tempVert = *stream + *currStreamPts;
				tempIndx = *indices + *currIndex;

				tempVert[0] = points[0].coord;
				tempVert[1] = tp0;
				_FlattenCircle(&tempVert[0], radius, 1, 359, context->tessContext.strokeJoinStep, tempVert + 2);
				numPoints += 1;

				for (i = 0; i < numPoints - 1; i++)
				{
					tempIndx[i * 3] = (_VGuint16)(*currStreamPts);
					tempIndx[i * 3 + 1] = (_VGuint16)(*currStreamPts + i + 1);
					tempIndx[i * 3 + 2] = (_VGuint16)(*currStreamPts + i + 2);
				}
				/* The last triangle. */
				tempIndx[i * 3] = (_VGuint16)(*currStreamPts);
				tempIndx[i * 3 + 1] = (_VGuint16)(*currStreamPts + i + 1);
				tempIndx[i * 3 + 2] = (_VGuint16)(*currStreamPts + 1);

				*currStreamPts += numPoints + 1;
				*currIndex += numPoints * 3;
				OVG_SAFE_FREE(context->os, pointsCopy);

				gcmFOOTER_NO();
				return;
            }else
            if (cap == VG_CAP_SQUARE)
            {
                _VGTessPoint        pts[3];
                _VGVector2			v0, v1;

                pts[1] = points[0];

                /* Using tangents. */
                v0 = points[0].inTan;
                v1 = points[0].outTan;

                pts[0].coord.x = pts[1].coord.x - TESS_MUL(radius, v0.x);
                pts[0].coord.y = pts[1].coord.y - TESS_MUL(radius, v0.y);
                pts[0].inTan = pts[0].outTan = v0;
                pts[2].coord.x = pts[1].coord.x + TESS_MUL(radius, v1.x);
                pts[2].coord.y = pts[1].coord.y + TESS_MUL(radius, v1.y);
                pts[2].inTan = pts[2].outTan = v1;

                if ((v0.x != v1.x) || (v0.y != v1.y))
                {
					VGCapStyle style = context->strokeCapStyle;
					context->strokeCapStyle = VG_CAP_BUTT;
					_ConstructStroke(context, pts,
											 3,
											 pathClosed,
											 streamPipe
											 );
					context->strokeCapStyle = style;

					OVG_SAFE_FREE(context->os, pointsCopy);
					gcmFOOTER_NO();
					return;
                }
                else
                {
                    /* 3 points in one line. */
                    pts[1] = pts[2];
					_ConstructStrokeBody(context, width,
												pts,
												streamPipe
												);
					OVG_SAFE_FREE(context->os, pointsCopy);
					gcmFOOTER_NO();
					return;
                }
            }
            else
            {
				OVG_SAFE_FREE(context->os, pointsCopy);
				gcmFOOTER_NO();
                return;
            }
        }
        else
        {
			OVG_SAFE_FREE(context->os, pointsCopy);
			gcmFOOTER_NO();
            return;
        }
    }

    /* Get rid of the last point if it is the same as the first point. */
    if (pathClosed && VECTOR_EQUAL(points[0].coord, points[numPoints - 1].coord)
        && (points[0].flags == POINT_FLAT && points[numPoints - 1].flags == POINT_FLAT))
    {
        --numPoints;
    }

    numLines = pathClosed ? numPoints : numPoints - 1;
    tempPoints = points;
    i = 0;

    /* Tessellate the first line. */
  if (!pathClosed)
    {
		_TessellateFirstLine(context, tempPoints,
												numLines == 1,
												pathClosed,
												streamPipe
												);

    }
    else
    {
        _VGTessPoint tp[4];
        tp[0] = points[0];
        tp[1] = points[1];
        if (VECTOR_EQUAL(points[0].coord, points[numPoints - 1].coord))
        {
            tp[2] = points[numPoints - 2];
            tp[3] = points[numPoints - 1];
        }
        else
        {
            tp[2] = points[numPoints - 1];
            tp[3] = points[0];
        }
		_TessellateFirstLine(context, tp,
												numLines == 1,
												pathClosed,
												streamPipe
												);

    }

  /* Tessellate the middle lines. */
    for (j = 2; j < numLines; ++j)
    {
        _TessellateMidLine(context, tempPoints,
                                              tempPoints + 1,
											  streamPipe
											  );
        ++tempPoints;
    }

  if (!pathClosed)
    {
        /* Tessellate the last line. */
        if (numLines > 1)
        {
            _TessellateLastLine(context, tempPoints,
                                                   tempPoints + 1,
                                                   gcvNULL,
												   streamPipe
												   );
        }
    }
    else
    {
        /* Tessellate the last line. */
        if (numLines > 1)
        {
            _VGTessPoint tempPts[2];
            tempPts[0] = *(tempPoints + 1);
            tempPts[1] = *points;
            tempPts[1].flags = POINT_FLAT;
            _TessellateLastLine(context, tempPoints,
                                                   tempPts,
                                                   points,
												   streamPipe
												   );
        }
    }

	OVG_SAFE_FREE(context->os, pointsCopy);

	gcmFOOTER_NO();
}

typedef struct _smpList
{
    _VGTessPoint    points[2];
    gctINT32             numPoints;
    gctBOOL            toBeContinued;
    struct _smpList *next;
}   SIMPLE_LIST;

/* It returns the number of total points that compose these dashed line tContext->segments. */
gctINT32    _DivideIntoDashLines(
	_VGContext		*context,
    _VGTessPoint    point[],            /* Points of the flattened sub-path. */
    gctINT32        numPointsToDo,      /* The number of the points of the lines to process. */
    _VGTesstype     dashPattern[],      /* VG dash pattern definition. */
	gctBOOL			subPathClosed,
    gctINT32        dashCount,          /* VG dash count. */
    _VGTesstype     dashPhase,          /* VG dash phase. */
    _VGTesstype     *dashPhaseNext,     /* Dash phase for the next sub-path. If DASH_PHASE_RESET is TRUE, it is ignored. */
    gctINT32        *numDashSegs,
    _VGTessPoint    **resultPoints,     /* Points that the dahs generates */
    gctINT32        **countArray        /* The number of points each continus segment contains. */
    )
{
    gctINT32        i,j;
    gctINT32        numTotalPoints;
    gctINT32        dashingIndex;       /* For the current line, what dash we lie in. */
    _VGTesstype     dashingPhase;       /* For the current dashing, what's the phase we should start with. */
    _VGTesstype     dashLength;
    gctINT32        numSegs;
    _VGTesstype     lineLength;
    _VGVector2		nVec;
    _VGTessPoint    *points;
     SIMPLE_LIST    *headNode, *currentNode;

	 gcmHEADER_ARG("context=0x%x points=0x%x numPointsToDo=%d dashPattern=0x%x subPathClosed=%s dashCount=%d "
				   "dashPhase=%f dashPhaseNext=0x%x numDashSegs=0x%x resultPoints=0x%x countArray=0x%x",
					context,
					point,
					numPointsToDo,
					dashPattern,
					subPathClosed ? "TRUE" : "FALSE",
					dashCount,
					dashPhase,
					dashPhaseNext,
					numDashSegs,
					resultPoints,
					countArray);


    numTotalPoints  = 0;
    dashingIndex    = 0;
    dashingPhase    = 0;
    dashLength      = 0;
    numSegs         = 0;

	OVG_MALLOC(context->os, points, sizeof(_VGTessPoint) * (numPointsToDo + 1));
    OVG_MEMCOPY(points, point, sizeof(_VGTessPoint) * numPointsToDo);
    points[numPointsToDo] = points[0];
    ++numPointsToDo;

    /* Get the dashingIndex and dashingPhase. */
    dashCount &= ~0x00000001;       /* Ignore the last dash if the count is odd according to the spec @ p102 */
    for (i = 0; i < dashCount; ++i)
    {
        dashLength += dashPattern[i];
    }

    if (dashPhase > 0)           /* Clamp the dashPhase within the dash pattern. */
    {
        dashPhase -= TESS_MUL( TESS_FLOOR(TESS_DIV(dashPhase, dashLength)), dashLength );
    }
    else
    {
        while(dashPhase < 0)
            dashPhase += dashLength;
    }

    dashingPhase = dashPhase;
    for (i = 0; i < dashCount; ++i)     /* Find the proper dashing states. */
    {
        if ((dashPattern[i] > dashingPhase) || (dashPattern[i] == 0 && dashingPhase == 0))
        {
            dashingIndex = i;
            break;
        }
        else
        {
            dashingPhase -= dashPattern[i];
        }
    }

    /* Go through each line to divide the current line by the dash pattern.
       And for each line, go through all dashes avaliable before it reaches the end point.*/
	OVG_MALLOC(context->os, currentNode, sizeof(SIMPLE_LIST));
	OVG_MEMSET(currentNode, 0, sizeof(SIMPLE_LIST));
	headNode = currentNode;
    headNode->toBeContinued = gcvFALSE;
    numTotalPoints = 0;
    --numPointsToDo;
    for (i = 0; i < numPointsToDo; ++i)     /* Go through all lines between any adjacent 2 points. */
    {
        lineLength = points[i].distance2Next;   /* (_VGTesstype)SQRT(dVec.x * dVec.x + dVec.y * dVec.y); */
        nVec = points[i].outTan;

        while (lineLength > 0)
        {
            if ( (dashingIndex & 0x00000001) == 0 )     /* If the current dash phase lies in an "on" dash. */
            {
                if (dashPattern[dashingIndex] == 0)
                {
					OVG_MALLOC(context->os, currentNode->next, sizeof(SIMPLE_LIST));
					OVG_MEMSET(currentNode->next, 0, sizeof(SIMPLE_LIST));
                    currentNode = currentNode->next;
                    currentNode->next = gcvNULL;

                    currentNode->points[0].coord.x = points[i + 1].coord.x - TESS_MUL(lineLength, nVec.x);
                    currentNode->points[0].coord.y = points[i + 1].coord.y - TESS_MUL(lineLength, nVec.y);
                    currentNode->points[0].inTan = points[i].outTan;
                    currentNode->points[0].outTan = points[i + 1].inTan;
                    currentNode->toBeContinued = gcvFALSE;
                    currentNode->numPoints = 1;

                    numTotalPoints += 1;
                    ++dashingIndex;
                    dashingIndex %= dashCount;
                    dashingPhase = 0;
                    ++numSegs;
                }else
                if (lineLength >= dashPattern[dashingIndex] - dashingPhase)     /* If the current dash lies within the current line. */
                {
					OVG_MALLOC(context->os, currentNode->next, sizeof(SIMPLE_LIST));
					OVG_MEMSET(currentNode->next, 0, sizeof(SIMPLE_LIST));
                    currentNode = currentNode->next;
                    currentNode->next = gcvNULL;

					/* Determine the coordinate value and tangent value. */
                    currentNode->points[0].coord.x = points[i + 1].coord.x - TESS_MUL(lineLength, nVec.x);
                    currentNode->points[0].coord.y = points[i + 1].coord.y - TESS_MUL(lineLength, nVec.y);
                    if (lineLength == points[i].distance2Next)
                    {
						currentNode->points[0].coord = points[i].coord;
                        currentNode->points[0].inTan = points[i].inTan;
                    }
                    else
                    {
                        currentNode->points[0].inTan = points[i].outTan;
                    }
                    currentNode->points[0].outTan = points[i].outTan;

                    lineLength -= (dashPattern[dashingIndex] - dashingPhase);
                    currentNode->points[1].coord.x = points[i + 1].coord.x - TESS_MUL(lineLength, nVec.x);
                    currentNode->points[1].coord.y = points[i + 1].coord.y - TESS_MUL(lineLength, nVec.y);
                    currentNode->points[1].inTan = currentNode->points[1].outTan = points[i].outTan;

					/* Determine the point flag value. */
					if (dashingPhase == 0)
					{
						currentNode->points[0].flags = points[i].flags;
					}
					else
					{
						if (points[i].flags == POINT_SMOOTH ||
							points[i].flags == POINT_SMOOTH_OUT)
							currentNode->points[0].flags = POINT_SMOOTH;
						else
							currentNode->points[0].flags = POINT_FLAT;
					}
					if (VECTOR_EQUAL(currentNode->points[1].coord, points[i + 1].coord))
					{
						currentNode->points[1].flags = points[i + 1].flags;
					}
					else
					{
						if (points[i + 1].flags == POINT_SMOOTH ||
							points[i + 1].flags == POINT_SMOOTH_IN)
							currentNode->points[1].flags = POINT_SMOOTH;
						else
							currentNode->points[1].flags = POINT_FLAT;
					}

                    currentNode->toBeContinued = gcvFALSE;
                    currentNode->numPoints = 2;

                    numTotalPoints += 2;
                    ++dashingIndex;
                    dashingIndex %= dashCount;
                    dashingPhase = 0;
                    ++numSegs;
                }
                else                                            /* If the current dash covers more than the current line. */
                {
					OVG_MALLOC(context->os, currentNode->next, sizeof(SIMPLE_LIST));
					OVG_MEMSET(currentNode->next, 0, sizeof(SIMPLE_LIST));
                    currentNode = currentNode->next;
                    currentNode->next = gcvNULL;

					if (lineLength == points[i].distance2Next)
					{
						currentNode->points[0] = points[i];
					}
					else
					{
						currentNode->points[0].coord.x = points[i + 1].coord.x - TESS_MUL(lineLength, nVec.x);
						currentNode->points[0].coord.y = points[i + 1].coord.y - TESS_MUL(lineLength, nVec.y);
						currentNode->points[0].inTan = currentNode->points[0].outTan = points[i + 1].inTan;
					}
                    currentNode->points[1] = points[i + 1];
                    currentNode->toBeContinued = gcvTRUE;
                    currentNode->numPoints = 2;

					/* Determine the point flag value. */
					if (VECTOR_EQUAL(currentNode->points[0].coord, points[i].coord))
					{
						currentNode->points[0].flags = points[i].flags;
					}
					else
					{
						if (points[i].flags == POINT_SMOOTH ||
							points[i].flags == POINT_SMOOTH_OUT)
							currentNode->points[0].flags = POINT_SMOOTH;
						else
							currentNode->points[0].flags = POINT_FLAT;
					}
					currentNode->points[1].flags = points[i + 1].flags;

					dashingPhase += lineLength;
                    lineLength = 0;
                    ++numTotalPoints;
                    break;
                }
            }
            else                                        /* If the current dash phase lies in an "off" dash. */
            {
                if (lineLength >= dashPattern[dashingIndex] - dashingPhase)
                {
                    lineLength -= (dashPattern[dashingIndex] - dashingPhase);
                    ++dashingIndex;
                    dashingIndex %= dashCount;
                    dashingPhase = 0;
                }
                else
                {
                    dashingPhase += lineLength;
                    lineLength = 0;
                    break;
                }
            }
        }
    }

    if ( currentNode->toBeContinued == gcvTRUE )
    {
        ++numSegs;
        ++numTotalPoints;
        currentNode->toBeContinued = gcvFALSE;
    }

	/* If the subpath is closed, then the close-join is processed here. The last segment extends to overlap the first segment (maybe partially) to make the join. */
	if (subPathClosed)
	{
		if (headNode->next != gcvNULL &&
			VECTOR_EQUAL(currentNode->points[currentNode->numPoints - 1].coord, headNode->next->points->coord))
		{
			OVG_MALLOC(context->os, currentNode->next, sizeof(SIMPLE_LIST));
			currentNode->toBeContinued = gcvTRUE;
			currentNode = currentNode->next;

			currentNode->next = gcvNULL;
			currentNode->toBeContinued = gcvFALSE;
			currentNode->numPoints = headNode->next->numPoints;
			OVG_MEMCOPY(currentNode->points, headNode->next->points, sizeof(_VGTessPoint) * headNode->next->numPoints);
			++numSegs;
			++numTotalPoints;
		}
	}

    *dashPhaseNext = dashingPhase;
    for (i = 0; i < dashingIndex; ++i)
    {
        *dashPhaseNext += dashPattern[i];
    }

	/* Check if there's any result generated. */
	if (numTotalPoints == 0)
	{
		OVG_FREE(context->os, headNode);
		*resultPoints = gcvNULL;
		*countArray = gcvNULL;
		*numDashSegs = 0;

		gcmFOOTER_ARG("return=%d", 0);
		return 0;
	}

    /* Copy the result and do any finalizations. */
	OVG_MALLOC(context->os, *resultPoints, sizeof(_VGTessPoint) * numTotalPoints);
	OVG_MALLOC(context->os, *countArray, sizeof(gctINT32) * numSegs);
    currentNode = headNode->next;
    i = 0;
    j = 0;
    gcoOS_ZeroMemory(*countArray, sizeof(gctINT32) * numSegs);
    while (currentNode != gcvNULL)
    {
        OVG_FREE(context->os, headNode);
        headNode = currentNode;

        /* Copy the results. */
        (*resultPoints)[i++] = currentNode->points[0];
        ++(*countArray)[j];

        if (!currentNode->toBeContinued)
        {
            if (currentNode->numPoints == 2)    /* Another case is that numPoints == 1, a single point, which lies on dash phase with length being   0. */
            {
                (*resultPoints)[i++] = currentNode->points[1];
                ++(*countArray)[j];
            }
            ++j;
        }

        currentNode = currentNode->next;
    }

	*numDashSegs = j;
    OVG_SAFE_FREE(context->os, headNode);
    OVG_SAFE_FREE(context->os, points);

	gcmFOOTER_ARG("return=%d", numTotalPoints);
    return numTotalPoints;
}
/* End of Stroke Tessellation. */

/************************************************************ Triangluation Methods. */
_VGint16 *_Triangulation(
	_VGContext		*context,
    _VGVector2		**points,
    gctINT32        *pointsLength,
    gctINT32        loopLength[],
    gctINT32        loopLengthLength,
    gctBOOL         nonZero,
    gctINT32        *triangleCounts,
    gctINT32        *arrayLength
    )
{
    gctINT32        i = 0, loop = 0;
    gctINT32        *regionHead = gcvNULL;
    gctINT32        regionHeadLength;
    gctINT32        mountainsLength = 0, *mountainsLengths = gcvNULL;
	_VGTessellationContext	*tContext = &context->tessContext;
#if USE_TA
	gcoOS			os = context->os;
#endif

	/* Variables in loops. */
    gctINT32 fromPoint, pointHigh, lastEdge;

    gctINT32    headCount;
    gctINT32    **tempHead;
    gctINT32    tempHeadLength;
    gctINT32    *tempHeadLengths0;

	gcmHEADER_ARG("context=0x%x points=0x%x pointsLength=0x%x loopLength=0x%x loopLengthLength=%d nonZero=%s "
				  "triangleCounts=0x%x arrayLength=0x%x",
				  context, points, pointsLength, loopLength, loopLengthLength,
				  nonZero ? "TRUE" : "FALSE",
				  triangleCounts, arrayLength);

	if (triangleCounts != gcvNULL)
	{
		*triangleCounts = 0;
	}

	if (*pointsLength > 65535)	/* We can not handle too many vertices. */
	{
		gcmFOOTER_ARG("return=0x%x", gcvNULL);
		return gcvNULL;
	}

	tContext->vertices = *points;
    tContext->verticesLength = *pointsLength;
	tContext->totalPoints = *pointsLength;
	tContext->originalTotalPoints = tContext->totalPoints;

	tContext->edgeInLength = tContext->totalPoints;
    tContext->edgeOutLength = tContext->totalPoints;
	tContext->numLoops = loopLengthLength;                                                  /* loopLength.Length; */
#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeHigh, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeLow, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeUpDown, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->loopStart, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeInLengths, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeOutLengths, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeIn, sizeof(gctINT32*) * tContext->totalPoints, 2)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeOut, sizeof(gctINT32*) * tContext->totalPoints, 2))
	{
		goto on_memory;
	}

	for (i = 0; i < tContext->totalPoints; i++)
	{
        tContext->edgeIn[i][0] = 0;
        tContext->edgeInLengths[i] = 1;

        tContext->edgeOut[i][0] = 0;
        tContext->edgeOutLengths[i] = 1;
	}
#else
    gcmASSERT(tContext->edgeHigh == gcvNULL && tContext->edgeLow == gcvNULL);
	OVG_MALLOC(context->os, tContext->edgeHigh, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeHigh, 0, sizeof(gctINT32) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->edgeLow, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeLow, 0, sizeof(gctINT32) * tContext->totalPoints);

    gcmASSERT(tContext->edgeIn == gcvNULL && tContext->edgeOut == gcvNULL && tContext->edgeUpDown == gcvNULL);

	OVG_MALLOC(context->os, tContext->edgeIn, sizeof(gctINT32*) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeIn, 0, sizeof(gctINT32*) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->edgeInLengths, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeInLengths, 0, sizeof(gctINT32) * tContext->totalPoints);

	OVG_MALLOC(context->os, tContext->edgeOut, sizeof(gctINT32*) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeOut, 0, sizeof(gctINT32*) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->edgeOutLengths, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeOutLengths, 0, sizeof(gctINT32) * tContext->totalPoints);

	OVG_MALLOC(context->os, tContext->edgeUpDown, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeUpDown, 0, sizeof(gctINT32) * tContext->totalPoints);
	for (i = 0; i < tContext->totalPoints; i++)
	{
		OVG_MALLOC(context->os, tContext->edgeIn[i], sizeof(gctINT32));
        tContext->edgeIn[i][0] = 0;
        tContext->edgeInLengths[i] = 1;

		OVG_MALLOC(context->os, tContext->edgeOut[i], sizeof(gctINT32));
        tContext->edgeOut[i][0] = 0;
        tContext->edgeOutLengths[i] = 1;
	}

    gcmASSERT(tContext->loopStart == gcvNULL);
	OVG_MALLOC(context->os, tContext->loopStart, sizeof(gctINT32) * (tContext->numLoops + 1));
    OVG_MEMSET(tContext->loopStart, 0, sizeof(gctINT32) * (tContext->numLoops + 1));
#endif

	tContext->loopStart[0] = 0;
	for (loop = 0; loop < tContext->numLoops; loop++)
	{
        fromPoint = 0;
		pointHigh = 0;
		lastEdge = 0;

		tContext->loopStart[loop + 1] = tContext->loopStart[loop] + loopLength[loop];
		fromPoint = tContext->loopStart[loop];

		for (i = tContext->loopStart[loop]; i < tContext->loopStart[loop + 1] - 1; i++)
		{
			pointHigh = _PointHigh3Way(tContext, i, i + 1);
			if (pointHigh == 0)
			{
				tContext->edgeHigh[i] = -1;
				tContext->edgeLow[i] = -1;
				tContext->edgeIn[i + 1][0] = -1;
				tContext->edgeOut[i + 1][0] = -1;
			}
			else
			{
				tContext->edgeIn[i + 1][0] = i;
				tContext->edgeOut[fromPoint][0] = i;
				if (pointHigh > 0)
				{
					tContext->edgeHigh[i] = fromPoint;
					tContext->edgeLow[i] = i + 1;
					tContext->edgeUpDown[i] = 1;
				}
				else
				{
					tContext->edgeHigh[i] = i + 1;
					tContext->edgeLow[i] = fromPoint;
					tContext->edgeUpDown[i] = -1;
				}
				fromPoint = i + 1;
			}
		}

        pointHigh = _PointHigh3Way(tContext, tContext->loopStart[loop + 1] - 1, tContext->loopStart[loop]);
		lastEdge = tContext->loopStart[loop + 1] - 1;

        if (pointHigh > 0)
		{
			tContext->edgeHigh[lastEdge] = fromPoint;
			tContext->edgeLow[lastEdge] = tContext->loopStart[loop];
			tContext->edgeUpDown[lastEdge] = 1;
			tContext->edgeIn[tContext->loopStart[loop]][0] = lastEdge;
			tContext->edgeOut[fromPoint][0] = lastEdge;
		}
		else if (pointHigh < 0)
		{
			tContext->edgeHigh[lastEdge] = tContext->loopStart[loop];
			tContext->edgeLow[lastEdge] = fromPoint;
			tContext->edgeUpDown[lastEdge] = -1;
			tContext->edgeIn[tContext->loopStart[loop]][0] = lastEdge;
			tContext->edgeOut[fromPoint][0] = lastEdge;
		}
		else
		{
			tContext->edgeHigh[lastEdge] = -1;
			tContext->edgeLow[lastEdge] = -1;
			tContext->edgeOut[fromPoint][0] = tContext->edgeOut[tContext->loopStart[loop]][0];
			if (tContext->edgeUpDown[tContext->edgeOut[tContext->loopStart[loop]][0]] == 1)
			{
				tContext->edgeHigh[tContext->edgeOut[tContext->loopStart[loop]][0]] = fromPoint;
			}
			else
			{
				tContext->edgeLow[tContext->edgeOut[tContext->loopStart[loop]][0]] = fromPoint;
			}
			tContext->edgeIn[tContext->loopStart[loop]][0] = -1;
			tContext->edgeOut[tContext->loopStart[loop]][0] = -1;
		}
	}

	if (tContext->loopStart[tContext->numLoops] != tContext->totalPoints)
	{
		_ResetTriangulateStates(context);
		gcmFOOTER_ARG("return=0x%x", tContext->triangles);
		return tContext->triangles;
	}

	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		_Trapezoidation(context))
	{
		goto on_memory;
	}

#if !USE_TA
    OVG_SAFE_FREE(context->os, tContext->tree);
    OVG_SAFE_FREE(context->os, tContext->pointsAdded);
    OVG_SAFE_FREE(context->os, tContext->pointsTreePosition);
#endif
    tContext->treeLength = 0;
    tContext->regionsLength = tContext->regionCounter + 1;

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->trapezoidDepth, sizeof(gctINT32) * tContext->regionsLength, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->nextOdd, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->nextEven, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeDirection, sizeof(gctINT32) * tContext->totalPoints, 1))
	{
		goto on_memory;
	}
#else
    gcmASSERT(tContext->trapezoidDepth == gcvNULL && tContext->nextOdd == gcvNULL && tContext->nextEven == gcvNULL && tContext->edgeDirection == gcvNULL);
	OVG_MALLOC(context->os, tContext->trapezoidDepth, sizeof(gctINT32) * tContext->regionsLength);
    OVG_MEMSET(tContext->trapezoidDepth, 0, sizeof(gctINT32) * tContext->regionsLength);
	OVG_MALLOC(context->os, tContext->nextOdd, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->nextOdd, 0, sizeof(gctINT32) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->nextEven, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->nextEven, 0, sizeof(gctINT32) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->edgeDirection, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeDirection, 0, sizeof(gctINT32) * tContext->totalPoints);
#endif
	for (i = 0; i < tContext->totalPoints; i++)
	{
		tContext->nextOdd[i] = -1;
		tContext->nextEven[i] = -1;
	}
	for (i = 0; i < tContext->regionsLength ; i++)
	{
		tContext->trapezoidDepth[i] = -999;
	}

	regionHead = _SetTrapezoidDepth(context, 0, 0, &regionHeadLength);           /*  Set for the unbound area */

	/*  plus or minus 1 when crossing a loop based on loop's direction */
	while (regionHeadLength > 0)
	{
        headCount = 0;
        tempHead = gcvNULL;
        tempHeadLength = 0;
        tempHeadLengths0 = gcvNULL;

#if LOCAL_MEM_OPTIM
		_AllocateIntp(tContext->IntpPool, &tempHead, regionHeadLength);
#else
		OVG_MALLOC(context->os, tempHead, sizeof(gctINT32*) * regionHeadLength);
#endif
        OVG_MEMSET(tempHead, 0, sizeof(gctINT32*) * regionHeadLength);
        tempHeadLength = regionHeadLength;
#if LOCAL_MEM_OPTIM
		_AllocateInt(tContext->IntPool, &tempHeadLengths0, regionHeadLength);
#else
		OVG_MALLOC(context->os, tempHeadLengths0, sizeof(gctINT32) * regionHeadLength);
#endif
        OVG_MEMSET(tempHeadLengths0, 0, sizeof(gctINT32) * regionHeadLength);

        headCount = 0;
        for (i = 0; i < regionHeadLength; i++)
		{
			tempHead[i] = _SetTrapezoidDepth(context, regionHead[i * 2 + 0],
                                             regionHead[i * 2 + 1], &tempHeadLengths0[i]);
			headCount += tempHeadLengths0[i];
		}

#if LOCAL_MEM_OPTIM
		if (regionHead != gcvNULL)
		{
			_FreeInt(tContext->IntPool, regionHead);
			regionHead = gcvNULL;
		}
#else
        OVG_SAFE_FREE(context->os, regionHead);
#endif
		if (headCount > 0)
		{
#if LOCAL_MEM_OPTIM
			_AllocateInt(tContext->IntPool, &regionHead, headCount * 2);
#else
			OVG_MALLOC(context->os, regionHead, sizeof(gctINT32) * headCount * 2);
#endif
			OVG_MEMSET(regionHead, 0, sizeof(gctINT32) * headCount * 2);
		}
        regionHeadLength = headCount;

		headCount = 0;
		for (i = 0; i < tempHeadLength; i++)
		{
			if (tempHeadLengths0[i] > 0)
			{
				OVG_MEMCOPY(regionHead + headCount, tempHead[i], sizeof(gctINT32*) * tempHeadLengths0[i] * 2);
				headCount += tempHeadLengths0[i] * 2;
#if LOCAL_MEM_OPTIM
				if (tempHead[i] != gcvNULL)
				{
					_FreeInt(tContext->IntPool, tempHead[i]);
					tempHead[i] = gcvNULL;
				}
#else
				OVG_SAFE_FREE(context->os, tempHead[i]);
#endif
			}
		}
#if LOCAL_MEM_OPTIM
		if (tempHead != gcvNULL)
		{
			_FreeIntp(tContext->IntpPool, tempHead);
			tempHead = gcvNULL;
		}
		if (tempHead != gcvNULL)
		{
			_FreeInt(tContext->IntPool, tempHeadLengths0);
			tempHeadLengths0 = gcvNULL;
		}
#else
        OVG_SAFE_FREE(context->os, tempHead);
        OVG_SAFE_FREE(context->os, tempHeadLengths0);
#endif
	}

	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		_MakeMountains(context, 1, &mountainsLength, &mountainsLengths))         /* odd numbered regions */
	{
		goto on_memory;
	}

	if (tContext->errorHandling)
	{
		OVG_MALLOC(context->os, tContext->triangles, sizeof(_VGint16) * tContext->totalPoints * 3 * 3 * 5);
		/* Out-of-Memory. */
		if (tContext->triangles == gcvNULL)
		{
			goto on_memory;
		}
		OVG_MEMSET(tContext->triangles, 0, sizeof(_VGint16) * tContext->totalPoints * 3 * 3 * 5);
	}
	else
	{
		OVG_MALLOC(context->os, tContext->triangles, sizeof(_VGint16) * tContext->totalPoints * 3 * 3);
		/* Out-of-Memory. */
		if (tContext->triangles == gcvNULL)
		{
			goto on_memory;
		}
		OVG_MEMSET(tContext->triangles, 0, sizeof(_VGint16) * tContext->totalPoints * 3 * 3);
	}

	tContext->triangleCounter = 0;
	for (i = 0; i < mountainsLength; i++)
	{
		/* Out-of-Memory. */
		if (gcvSTATUS_OK !=
			_MoveMountain(context, tContext->mountains[i], mountainsLengths[i]))
		{
			goto on_memory;
		}
#if LOCAL_MEM_OPTIM
		if (mountains[i] != gcvNULL);
		{
			_FreeInt(tContext->IntPool, mountains[i]);
			mountains[i] = gcvNULL;
		}
#else
#endif
	}
#if LOCAL_MEM_OPTIM
	if (mountains != gcvNULL)
	{
		_FreeIntp(tContext->IntpPool, mountains);
		mountains = gcvNULL;
	}
	if (mountainsLengths != gcvNULL)
	{
		_FreeInt(tContext->IntPool, mountainsLengths);
		mountainsLengths = gcvNULL;
	}
#else
    OVG_SAFE_FREE(context->os, mountainsLengths);
#endif

	if (nonZero)	 /* even numbered regions */
	{
		for (i = 0; i < tContext->totalPoints; i++)
		{
			tContext->edgeDirection[i] = -tContext->edgeDirection[i];
		}

		/* Out-of-Memory. */
		if (gcvSTATUS_OK !=
			_MakeMountains(context, 0, &mountainsLength, &mountainsLengths))
		{
			goto on_memory;
		}

		for (i = 0; i < mountainsLength; i++)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				_MoveMountain(context, tContext->mountains[i], mountainsLengths[i]))
			{
				goto on_memory;
			}
#if LOCAL_MEM_OPTIM
			if (mountains[i] != gcvNULL)
			{
				_FreeInt(tContext->IntPool, mountains[i]);
				mountains[i] = gcvNULL;
			}
#else
#endif
		}
#if LOCAL_MEM_OPTIM
		if (mountains != gcvNULL)
		{
			_FreeIntp(tContext->IntpPool, mountains);
			mountains = gcvNULL;
		}
		if (mountainsLengths != gcvNULL)
		{
			_FreeInt(tContext->IntPool, mountainsLengths);
			mountainsLengths = gcvNULL;
		}
#else
        OVG_SAFE_FREE(context->os, mountainsLengths);
#endif
	}

    *points = tContext->vertices;
	if (triangleCounts != gcvNULL)
	{
		*triangleCounts = tContext->triangleCounter;
	}
    *pointsLength = tContext->totalPoints;

    tContext->vertices = gcvNULL;
    _ResetTriangulateStates(context);

#if LOCAL_MEM_OPTIM
	if (regionHead != gcvNULL)
	{
		_FreeInt(tContext->IntPool, regionHead);
		regionHead = gcvNULL;
	}
#else
    OVG_SAFE_FREE(context->os, regionHead);
#endif

	gcmFOOTER_ARG("return=0x%x", tContext->triangles);
	return tContext->triangles;

on_memory:
	OVG_SAFE_FREE(context->os, regionHead);
	OVG_SAFE_FREE(context->os, tContext->triangles);
	_VGTessellationContextDtor(context);
	_VGTessellationContextCtor(os, tContext);

	gcmFOOTER_ARG("return=0x%x", gcvNULL);
	return gcvNULL;
}

/* This routine sets up the tree structure of points, edges, and trapezoids. The basic idea can be found in
* various publications. The way to treat intersections, however, is my own. The idea is to break the edges at
* each intersection, and to put two points there, distinguished (and ordered) by a third dimension. Because of
* the ordering, there is a tiny (actually with 0 area) trapezoid at each intersection.
* Because of the extra trapezoids and related structures, there is a penalty in performance. Also it cannot
* easily handle 2 input points sharing the same location, and more than 2 edges intersecting at one point.
* An alternative is not to distinguish the points at all, but allow a vertex to be used by more than 2 edges.
* This way there will be no extra trapezoids introduced, but we will have more complicated structure at each
* point. And there will also be more work when calculating trapezoid depth and tracing mountains, because we
* can no longer rely on the fact that each vertex has only two edges.
*/
gceSTATUS _Trapezoidation(
	_VGContext	*context
)
{
    gctINT32 start = 0, gap = 0, i = 0;
	_VGTessellationContext	*tContext = &context->tessContext;
	gceSTATUS	status = gcvSTATUS_OK;
#if USE_TA
	gcoOS	os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x", context);

    tContext->regionsLength = tContext->totalPoints * 2 + 1;
#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		TA_Malloc(os, (void**)&tContext->regions, sizeof(_VGTrapezoid) * tContext->regionsLength, 1))
	{
		status = gcvSTATUS_OUT_OF_MEMORY;
		gcmFOOTER();
		return status;
	}
#else
	OVG_MALLOC(context->os, tContext->regions, sizeof(_VGTrapezoid) * tContext->regionsLength);
    OVG_MEMSET(tContext->regions, 0, sizeof(_VGTrapezoid) * tContext->regionsLength);
#endif
	tContext->regionCounter = 0;
	tContext->regions[0].upperVertex = -1;	/* infinity */
	tContext->regions[0].lowerVertex  = -1;
	tContext->regions[0].leftEdge = -1;
	tContext->regions[0].rightEdge = -1;
	tContext->regions[0].treeNode = 0;		/* tree top */

    tContext->treeLength = tContext->totalPoints * 8;
#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		TA_Malloc(os, (void**)&tContext->tree, sizeof(_VGTreeNode) * tContext->treeLength, 1))
	{
		status = gcvSTATUS_OUT_OF_MEMORY;
		gcmFOOTER();
		return status;
	}
#else
	OVG_MALLOC(context->os, tContext->tree, sizeof(_VGTreeNode) * tContext->treeLength);
    OVG_MEMSET(tContext->tree, 0, sizeof(_VGTreeNode) * tContext->treeLength);
#endif
	tContext->treeTop = 0;
	tContext->tree[0].type = 0;
	tContext->tree[0].objectIndex = 0;		/* region 0 */

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->pointsAdded, sizeof(gctBOOL) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->edgeAdded, sizeof(gctBOOL) * tContext->totalPoints, 1)		||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->pointsTreePosition, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->regionBelow, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->regionBelow2, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->regionBelow3Lengths, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->regionAboveLengths, sizeof(gctINT32) * tContext->totalPoints, 1)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->regionBelow3, sizeof(gctINT32*) * tContext->totalPoints, 2)	||
		gcvSTATUS_OK != TA_Malloc(os, (void**)&tContext->regionAbove, sizeof(gctINT32*) * tContext->totalPoints, 2))
	{
		status = gcvSTATUS_OUT_OF_MEMORY;
		gcmFOOTER();
		return status;
	}
#else
	OVG_MALLOC(context->os, tContext->pointsAdded, sizeof(gctBOOL) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->edgeAdded, sizeof(gctBOOL) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->pointsTreePosition, sizeof(gctINT32) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->regionBelow, sizeof(gctINT32) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->regionBelow2, sizeof(gctINT32) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->regionBelow3, sizeof(gctINT32*) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->regionBelow3Lengths, sizeof(gctINT32) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->regionAbove, sizeof(gctINT32*) * tContext->totalPoints);
	OVG_MALLOC(context->os, tContext->regionAboveLengths, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->pointsAdded, gcvFALSE, sizeof(gctBOOL) * tContext->totalPoints);
    OVG_MEMSET(tContext->edgeAdded, gcvFALSE, sizeof(gctBOOL) * tContext->totalPoints);
    OVG_MEMSET(tContext->pointsTreePosition, 0, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->regionBelow, 0, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->regionBelow2, 0, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->regionBelow3Lengths, 0, sizeof(gctINT32) * tContext->totalPoints);
    OVG_MEMSET(tContext->regionAboveLengths, 0, sizeof(gctINT32) * tContext->totalPoints);
#endif
    tContext->regionBelow3Length = tContext->totalPoints;
    tContext->regionAboveLength = tContext->totalPoints;
	for (i = 0; i < tContext->totalPoints; i++)
	{
#if USE_TA
#else
		OVG_MALLOC(context->os, tContext->regionBelow3[i], sizeof(gctINT32));
		OVG_MALLOC(context->os, tContext->regionAbove[i], sizeof(gctINT32));
#endif
        tContext->regionBelow3[i][0] = 0;
        tContext->regionBelow3Lengths[i] = 1;
        tContext->regionAbove[i][0] = 0;
        tContext->regionAboveLengths[i] = 1;
	}

	for (i = 0; i < tContext->originalTotalPoints; i++)
	{
		if (tContext->edgeUpDown[i] == 0)
		{
			tContext->edgeAdded[i] = gcvTRUE;
		}
        else if (tContext->vertices[tContext->edgeHigh[i]].y == tContext->vertices[tContext->edgeLow[i]].y)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				_AddEdge(context, i))
			{
				status = gcvSTATUS_OUT_OF_MEMORY;
				gcmFOOTER();
				return status;
			}
			tContext->edgeAdded[i] = gcvTRUE;
		}
	}

    /* The original algorithm asks for a random ordering. We opted for a predetermined "Going around" ordering. */
	gap = 18;
	while (gap < tContext->totalPoints)
	{
		gap <<= 1;
	}

	while (gap > 9)		/* Add 1/9 of the edges */
	{
		for (start = gap / 2; start < tContext->originalTotalPoints; start += gap)
		{
			if (!tContext->edgeAdded[start] != 0)
			{
				/* Out-of-Memory. */
				if (gcvSTATUS_OK !=
					_AddEdge(context, start))
				{
					status = gcvSTATUS_OUT_OF_MEMORY;
					gcmFOOTER();
					return status;
				}
			}
		}
		gap >>=1;
	}

	if (!tContext->edgeAdded[0] != 0)
	{
		/* Out-of-Memory. */
		if (gcvSTATUS_OK !=
	        _AddEdge(context, 0))
		{
			status = gcvSTATUS_OUT_OF_MEMORY;
			gcmFOOTER();
			return status;
		}
	}

	for (start = 3; start < tContext->originalTotalPoints; start += 9)	/* Add 1/6 of the points */
	{
		/* TracePoint(start - 2, 2); */
		if (!tContext->edgeAdded[start] != 0)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
		        _AddEdge(context, start))
			{
				status = gcvSTATUS_OUT_OF_MEMORY;
				gcmFOOTER();
				return status;
			}
		}
	}
	for (start = 6; start < tContext->originalTotalPoints; start += 9)
	{
		/* TracePoint(start - 2, 2); */
		if (!tContext->edgeAdded[start] != 0)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
		        _AddEdge(context, start))
			{
				status = gcvSTATUS_OUT_OF_MEMORY;
				gcmFOOTER();
				return status;
			}
		}
	}

	for (start = 1; start < tContext->originalTotalPoints; start += 3)	/* Add 1/3 of the points */
	{
		if (!tContext->edgeAdded[start] != 0)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
		        _AddEdge(context, start))
			{
				status = gcvSTATUS_OUT_OF_MEMORY;
				gcmFOOTER();
				return status;
			}
		}
	}

	for (start = 2; start < tContext->originalTotalPoints; start += 3)	/* Add 1/3 of the points */
	{
		if (!tContext->edgeAdded[start] != 0)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
	            _AddEdge(context, start))
			{
				status = gcvSTATUS_OUT_OF_MEMORY;
				gcmFOOTER();
				return status;
			}
		}
	}

	gcmFOOTER();
	return status;
}

gctBOOL    _PointHigh(
	_VGTessellationContext	*tContext,
    gctINT32        p1,
    gctINT32        p2
    )
{	/* When same height, left is considered higher */
	if (p1 == -1)		/* -1 is -infinity when apears here */
	{
		return gcvFALSE;
	}
	else if (p2 == -1)
	{
		return gcvTRUE;
	}
	else
	{
		gctBOOL r = (tContext->vertices[p1].y > tContext->vertices[p2].y) ||
               ((tContext->vertices[p1].y == tContext->vertices[p2].y) &&
                (tContext->vertices[p1].x < tContext->vertices[p2].x));

		return r;
    }
}

gctINT32    _PointHigh3Way(
	_VGTessellationContext	*tContext,
    gctINT32        p1,
    gctINT32        p2
    )
{	/* When same height, left is considered higher */
	if (p1 == -1)		/* -1 is -infinity when apears here */
	{
		return -1;
	}
	else if (p2 == -1)
	{
		return 1;
	}
	else if (tContext->vertices[p1].y > tContext->vertices[p2].y)
	{
		return 1;
	}
	else if (tContext->vertices[p2].y > tContext->vertices[p1].y)
	{
		return -1;
	}
	else
	{
		gctINT32 r = (tContext->vertices[p1].x < tContext->vertices[p2].x) ?
                1 : ((tContext->vertices[p1].x == tContext->vertices[p2].x) ? 0 : -1);

		return r;
	}
}

/* The general procedure of inserting an edge to the structure of already inserted vertices and edges is:
* 1) If the top vertex is not yet inserted, find the trapezoid it is located in, and insert it there. When
*    doing so, the trapezoid is horizontally divided in 2.
* 2) Each trapezoid that the edge trespasses is divided vertically in 2. But only 1 new trapezoid is created,
*    in the top most region. In other regions, the current region is shrunk, either to the left or right, and
*    on the other side the top region is extended down.
* 3) When the bottom region is reached, if the bottom vertex is not yet added, it is added first before the
*    region is divided by the edge. Since we are already in the region, we do not need to locate the vertex.
* When there are intersections, each of the 3 steps is complicated by the checking of intersections.
* 1) When inserting the top point, we need to check whether the point is on an already-inserted edge. If so,
*    the edge is broken in 2, and the edge-vertex relation rearanged, if necessary.
* 2) In each region, we need to check whether the current edge intersects with the left or right side of the
*    trapezoid. Also the intersection can happen in the middle of the region (when the intersection is edge-
*    to-edge), or at the bottom (when it is edge-to-vertex). Either way, the current edge is broken in 2, and
*    the tracing of the top half is ended at this step. A new insersion of the bottom half of the edge will
*    start there after.
* 3) At the end of an insersion, we again check whether the bottom vertex is on an exsting edge, similar to
*    the top vertex.
*/
gceSTATUS _AddEdge(
	_VGContext	*context,
    gctINT32        index
    )
{
    gctINT32 i;
	gctINT32 currRegion, currPoint, currPosition;
	/* regions to the left and right of the edge as it is drawn down */
	gctINT32 leftRegion, rightRegion, direction;
	gctINT32 leftIntersectionStatus;		/* 0 = not checked, 1 = has left edge to edge intersection, 2 = no intersection */
										        /* 3 = point to edge intersection, */
	gctINT32 rightIntersectionStatus;	    /* 0 = not checked, 1 = has right edge to edge intersection, 2 = no intersection */
	/* For intersection handling */
    _VGVector2	leftIntersection, rightIntersection;
    gctBOOL     toContinue;

	/* Variables in Loops. */
	gctINT32 nextPoint, nextRegion, nextDirection;
	gctINT32 currIntersection;
    _VGVector2	leftPoint, rightPoint;
	gctINT32 leftIndex, rightIndex;
	gctINT32 otherRegionsLength;
	gctINT32 *otherRegions;
	gctINT32 newEdge2;
	_VGTessellationContext	*tContext = &context->tessContext;
#if USE_TA
	gcoOS	os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x index=%d", context, index);

	do{
		i = 0;
		currRegion = 0, currPoint = 0, currPosition = 0;
		leftRegion = 0, rightRegion = 0, direction = 0;
		leftIntersectionStatus = 0;
		rightIntersectionStatus = 0;
		leftIntersection.x = leftIntersection.y = rightIntersection.x = rightIntersection.y = 0;
		toContinue = gcvFALSE;

		currPoint = tContext->edgeHigh[index];

		/* add top point and determine which region to go down */
		if (!tContext->pointsAdded[currPoint])
		{
			gctINT32 status = 0;
			status = _AddPoint(context, currPoint);
			/* Out-of-Memory. */
			if (status == -9999)
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}

			currRegion = tContext->regionBelow[currPoint];

			if (status == -1)	/* point is on edge */
			{ /* If point on edge, it is always added to left of edge*/
				otherRegions = _FindOtherRegions(context, tContext->regionAbove[currPoint][0], currPoint, 1, -1, &otherRegionsLength);
				/* Out-of-Memory. */
				if (otherRegions == gcvNULL)
				{
					gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

					return gcvSTATUS_OUT_OF_MEMORY;
				}
				if (gcvSTATUS_OK !=
					_BreakEdgeBunch(context, otherRegions, otherRegionsLength, currPoint, 0))
				{
					OVG_SAFE_FREE(context->os, otherRegions);
					gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

					return gcvSTATUS_OUT_OF_MEMORY;
				}
#if LOCAL_MEM_OPTIM
				if (otherRegions != gcvNULL)
				{
					_FreeInt(tContext->IntPool, otherRegions);
					otherRegions = gcvNULL;
				}
#else
				OVG_SAFE_FREE(context->os, otherRegions);
#endif
			}
			else if (status > 0)
			{/* duplicate points */
				gctINT32 duplicatePoint = status - 1;
				/* Out-of-Memory. */
				if (gcvSTATUS_OK !=
					_RearrangeEdges(context, duplicatePoint, currPoint, currPoint))
				{
					gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

					return gcvSTATUS_OUT_OF_MEMORY;
				}
			}
			currPoint = tContext->edgeHigh[index];	/* this may be changed */
		}

		if (tContext->regionBelow2[currPoint] == 0 ||
			_EdgeLeftRight(tContext, index, tContext->regions[tContext->regionBelow[currPoint]].rightEdge) == -1)
		{
			currRegion = tContext->regionBelow[currPoint];
		}
		else if (tContext->regionBelow3[currPoint][0] == 0)
		{
			currRegion = tContext->regionBelow2[currPoint];
		}
		else
		{
			currRegion = tContext->regionBelow2[currPoint];
			for (i = 0; i < tContext->regionBelow3Lengths[currPoint] ; i++)
			{
				if (_EdgeLeftRight(tContext, index, tContext->regions[tContext->regionBelow3[currPoint][i]].rightEdge) == -1)
				{
					currRegion = tContext->regionBelow3[currPoint][i];
					break;
				}
			}
		}

		/* tracing through the regions */
		while (currRegion != 0)
		{
			nextPoint = 0;
			nextRegion = 0;
			nextDirection = 0;

			/* Check intersection */
			currIntersection = 0;
            leftPoint.x = leftPoint.y = rightPoint.x = rightPoint.y = 0;

			if (leftIntersectionStatus == 0 &&
                tContext->regions[currRegion].leftEdge != -1)
			{
				leftIntersectionStatus = _CheckIntersection(tContext, index, tContext->regions[currRegion].leftEdge, &leftIntersection);
			}
			if (rightIntersectionStatus == 0 &&
                tContext->regions[currRegion].rightEdge != -1)
			{
				rightIntersectionStatus = _CheckIntersection(tContext, tContext->regions[currRegion].rightEdge, index, &rightIntersection);
			}
			if (leftIntersectionStatus == 1 &&
                leftIntersection.y >= tContext->vertices[tContext->regions[currRegion].lowerVertex].y)
			{	/* edge-to-edge intersection on left */
				currIntersection = -1;
				leftIntersectionStatus = 2;
				leftPoint = leftIntersection;
			}else
            if (rightIntersectionStatus == 1 &&
                rightIntersection.y > tContext->vertices[tContext->regions[currRegion].lowerVertex].y)
			{	/* edge-to-edge intersection on right */
				currIntersection = 1;
				rightIntersectionStatus = 2;
                leftPoint = rightIntersection;
			}else
            if (leftIntersectionStatus == 5 &&
                (tContext->edgeLow[index] != tContext->regions[currRegion].lowerVertex) &&
                leftIntersection.y >= tContext->vertices[tContext->regions[currRegion].lowerVertex].y)
			{	/* lower vertex of edge intersects left side */
				currIntersection = -2;
				leftIntersectionStatus = 2;
				leftPoint = leftIntersection;
			}else
            if (rightIntersectionStatus == 6 &&
                (tContext->edgeLow[index] != tContext->regions[currRegion].lowerVertex) &&
                rightIntersection.y > tContext->vertices[tContext->regions[currRegion].lowerVertex].y)
			{	/* lower vertex of edge intersects right side */
				currIntersection = 2;
				rightIntersectionStatus = 2;
				leftPoint = rightIntersection;
			}else
            if (leftIntersectionStatus == 6 &&
                leftIntersection.y >= tContext->vertices[tContext->regions[currRegion].lowerVertex].y)
			{	/* edge intersects lower vertex of left side */
				currIntersection = -3;
				leftIntersectionStatus = 2;
				leftPoint = leftIntersection;
			}else
            if (rightIntersectionStatus == 5 &&
                (rightIntersection.y >= tContext->vertices[tContext->regions[currRegion].lowerVertex].y ||
                (rightIntersection.y == tContext->vertices[tContext->regions[currRegion].lowerVertex].y &&
                 rightIntersection.x <= tContext->vertices[tContext->regions[currRegion].lowerVertex].x)))
			{	/* edge intersects lower vertex of right side */
				if (tContext->vertices[currPoint].x == rightIntersection.x &&
                    tContext->vertices[currPoint].y == rightIntersection.y &&
                    tContext->vertices[tContext->edgeLow[index]].y < rightIntersection.y)
				{	/* This should no longer happen as we eliminated co-position points */
					rightIntersectionStatus = 2;
					currIntersection = 0;
				}
				else
				{
					currIntersection = 3;
					rightIntersectionStatus = 2;
					leftPoint = rightIntersection;
				}
			}else
            if (leftIntersectionStatus == 3 &&
                tContext->regions[currRegion].lowerVertex == tContext->edgeLow[tContext->regions[currRegion].leftEdge] &&
				tContext->regions[currRegion].lowerVertex != tContext->edgeLow[index])
			{	/* edge and left side meet at lower end */
				currIntersection = 0;
				leftIntersectionStatus = 2;
				if (!tContext->pointsAdded[tContext->edgeLow[index]])
				{
					gctINT32 otherPoint = tContext->regions[currRegion].lowerVertex;
					/* Out-of-Memory. */
					if (gcvSTATUS_OK !=
						_RearrangeEdges(context, otherPoint, tContext->edgeLow[index], tContext->edgeLow[index]))
					{
						gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

						return gcvSTATUS_OUT_OF_MEMORY;
					}
				}
			}else
            if (rightIntersectionStatus == 3  &&
                tContext->regions[currRegion].lowerVertex == tContext->edgeLow[tContext->regions[currRegion].rightEdge] &&
				tContext->regions[currRegion].lowerVertex != tContext->edgeLow[index])
			{	/* edge and right side meet at lower end */
				currIntersection = 0;
				rightIntersectionStatus = 2;
				if (!tContext->pointsAdded[tContext->edgeLow[index]])
				{
					gctINT32 otherPoint = tContext->regions[currRegion].lowerVertex;
					/* Out-of-Memory. */
					if (gcvSTATUS_OK !=
						_RearrangeEdges(context, otherPoint, tContext->edgeLow[index], tContext->edgeLow[index]))
					{
						gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

						return gcvSTATUS_OUT_OF_MEMORY;
					}
				}
			}
			else
			{
				gctINT32 lowVertex = tContext->regions[currRegion].lowerVertex;
				if (lowVertex != -1 && lowVertex != tContext->edgeLow[index] &&
					(tContext->regions[currRegion].leftEdge == -1 ||
                     lowVertex != tContext->edgeLow[tContext->regions[currRegion].leftEdge]) &&
					(tContext->regions[currRegion].rightEdge == -1 ||
                     lowVertex != tContext->edgeLow[tContext->regions[currRegion].rightEdge]) &&
					_PointLeftRight(tContext, lowVertex, index) == 0)
				{
                    if (tContext->vertices[lowVertex].y > tContext->vertices[tContext->edgeLow[index]].y ||
						(tContext->vertices[lowVertex].y == tContext->vertices[tContext->edgeLow[index]].y &&
                         tContext->vertices[lowVertex].x < tContext->vertices[tContext->edgeLow[index]].x))
					{	/* edge intersects lower vertex of the region in the middle*/
						currIntersection = 5;
						leftPoint = tContext->vertices[tContext->regions[currRegion].lowerVertex];
					}else
                    if (tContext->vertices[lowVertex].y == tContext->vertices[tContext->edgeLow[index]].y &&
                        tContext->vertices[lowVertex].x == tContext->vertices[tContext->edgeLow[index]].x)
					{	/* replicate point */
						if (!tContext->pointsAdded[tContext->edgeLow[index]])
						{
							gctINT32 otherPoint = tContext->regions[currRegion].lowerVertex;
							/* Out-of-Memory. */
							if (gcvSTATUS_OK !=
								_RearrangeEdges(context, otherPoint, tContext->edgeLow[index], tContext->edgeLow[index]))
							{
								gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

								return gcvSTATUS_OUT_OF_MEMORY;
							}
						}
					}
				}
				else if (lowVertex != tContext->edgeLow[index] && _PointHigh(tContext, lowVertex, tContext->edgeLow[index]) &&
					(tContext->regions[currRegion].leftEdge != -1 && lowVertex == tContext->edgeLow[tContext->regions[currRegion].leftEdge]) &&
					(tContext->regions[currRegion].rightEdge != -1 && lowVertex == tContext->edgeLow[tContext->regions[currRegion].rightEdge]))
				{	/* error causing not detecting intersection */
					currIntersection = 5;
                    leftPoint = tContext->vertices[tContext->regions[currRegion].lowerVertex];
				}
			}

			if (currIntersection != 0)
			{
				leftIndex = 0;
				rightIndex = 0;
				if (tContext->totalPoints > tContext->verticesLength - 2)
				{
					/* Out-of-Memory. */
					if (gcvSTATUS_OK !=
						_ResizeArrays(context, tContext->totalPoints + gcmMAX(20, tContext->totalPoints / 100)))
					{
						gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

						return gcvSTATUS_OUT_OF_MEMORY;
					}
				}
				if (TESS_ABS((gctFLOAT)currIntersection) == 1)
				{	/* edge-to-edge, 2 new points needed, first one created here */
					tContext->totalPoints++;
					leftIndex = tContext->totalPoints-1;
					tContext->vertices[leftIndex] = leftPoint;
					tContext->edgeIn[leftIndex][0] = -1;
					tContext->edgeOut[leftIndex][0] = -1;
				}
				else if (TESS_ABS((gctFLOAT)currIntersection) == 2)
				{	/* edges ends on another edge */
					leftIndex = tContext->edgeLow[index];
				}
				else
				{	/* edge go through another vertex */
					leftIndex = tContext->regions[currRegion].lowerVertex;
				}
				/* locate other region */
				if (TESS_ABS((gctFLOAT)currIntersection) < 3)
				{
                    otherRegionsLength = 0;
					otherRegions = _FindOtherRegions(context, currRegion, leftIndex, (gctINT32)TESS_SIGN((long long)currIntersection), tContext->regions[currRegion].lowerVertex, &otherRegionsLength);

					/* Out-of-Memory. */
					if (otherRegions == gcvNULL)
					{
						gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

						return gcvSTATUS_OUT_OF_MEMORY;
					}

                    if (!tContext->pointsAdded[leftIndex])
					{
						tContext->pointsTreePosition[leftIndex] = tContext->regions[otherRegions[0]].treeNode;
						/* Out-of-Memory. */
						if (-9999 == _AddPoint(context, leftIndex))
						{
							gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

							return gcvSTATUS_OUT_OF_MEMORY;
						}
					}
					if (TESS_ABS((gctFLOAT)currIntersection) == 1)
					{
						/* Out-of-Memory. */
						if (gcvSTATUS_OK !=
							_BreakEdgeBunch(context, otherRegions, otherRegionsLength, leftIndex, -1))
						{
							gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

							return gcvSTATUS_OUT_OF_MEMORY;
						}

                        newEdge2 = tContext->totalPoints - 1;
						/* Out-of-Memory. */
						if (gcvSTATUS_OK !=
							_BreakOneEdge2(context, currRegion, index, leftIndex, newEdge2))
						{
							gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

							return gcvSTATUS_OUT_OF_MEMORY;
						}

						toContinue = gcvTRUE;	/* need to insert lower half */
					}
					else if (TESS_ABS((gctFLOAT)currIntersection) == 2)
					{
						/* Out-of-Memory. */
						if (gcvSTATUS_OK !=
							_BreakEdgeBunch(context, otherRegions, otherRegionsLength, leftIndex, 0))
						{
							gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

							return gcvSTATUS_OUT_OF_MEMORY;
						}
					}
#if LOCAL_MEM_OPTIM
					if (otherRegions != gcvNULL)
					{
						_FreeInt(tContext->IntPool, otherRegions);
						otherRegions = gcvNULL;
					}
#else
                    OVG_SAFE_FREE(context->os, otherRegions);
#endif
				}
				else
				{
					rightPoint = tContext->vertices[leftIndex];
					rightIndex = tContext->totalPoints;
					if (tContext->totalPoints >= tContext->verticesLength)
					{
						/* Out-of-Memory. */
						if (gcvSTATUS_OK !=
							_ResizeArrays(context, tContext->totalPoints + gcmMAX(20, tContext->totalPoints / 100)))
						{
							gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

							return gcvSTATUS_OUT_OF_MEMORY;
						}
					}
					tContext->totalPoints++;
					tContext->vertices[rightIndex] = rightPoint;
					/* Out-of-Memory. */
					if (gcvSTATUS_OK !=
						_BreakOneEdge2(context, currRegion, index, leftIndex, rightIndex))	/* Similar to BreakOneEdge but break an edge not yet fully inserted (current edge) */
					{
						gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

						return gcvSTATUS_OUT_OF_MEMORY;
					}
					toContinue = gcvTRUE;	/* need to insert lower half */
				}
			}

			/* tracing down */
			if (tContext->edgeLow[index] == tContext->regions[currRegion].lowerVertex)
			{	/* reached end and low vertex already inserted */
				nextPoint = tContext->edgeLow[index];
			}
			else if (_PointHigh(tContext, tContext->edgeLow[index], tContext->regions[currRegion].lowerVertex))
			{	/* reached end and low vertex not inserted */
				if (!tContext->pointsAdded[tContext->edgeLow[index]])	/* should not need this test */
				{
					tContext->pointsTreePosition[tContext->edgeLow[index]] = tContext->regions[currRegion].treeNode;
					/* Out-of-Memory. */
					if (-9999 ==
						_AddPoint(context, tContext->edgeLow[index]))
					{
						gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

						return gcvSTATUS_OUT_OF_MEMORY;
					}
				}
				nextPoint = tContext->edgeLow[index]; /* edges[index].lowVertex; */
			}
			else
			{
				/* a trapezoid may end below in 1 of 3 ways: left side reached low vertex, right side reached low vertex,
				 or there is a vertex between left and right sides.*/
				nextPoint = tContext->regions[currRegion].lowerVertex;
				if (tContext->regions[currRegion].leftEdge != -1 &&
					nextPoint == tContext->edgeLow[tContext->regions[currRegion].leftEdge])
				{	/* current region ended left */
					nextDirection = 1;
					nextRegion = tContext->regionBelow2[nextPoint] == 0 ? tContext->regionBelow[nextPoint] : tContext->regionBelow2[nextPoint];
				}else
				if (tContext->regions[currRegion].rightEdge != -1 &&
					nextPoint == tContext->edgeLow[tContext->regions[currRegion].rightEdge])
				{	/* current region ended right */
					nextDirection = -1;
					nextRegion = tContext->regionBelow[nextPoint];
				}
				else
				{	/* current region ended middle */
					nextDirection = -_PointLeftRight(tContext, nextPoint, index);	/* determine which side the edge goes to */
					if (nextDirection == 1)
					{	/* Go to left */
						nextRegion = tContext->regionBelow2[nextPoint] == 0 ? tContext->regionBelow[nextPoint] : tContext->regionBelow2[nextPoint];
					}
					else if (nextDirection == -1)
					{	/* go to right */
						nextRegion = tContext->regionBelow[nextPoint];
					}
				}
			}

			currPosition = tContext->regions[currRegion].treeNode;
			if (currPosition == -1)
			{	/* An intersection region between a pair of split points, not in tree */
			}
			else
			{ /* a regular region. Need to adjust tree */
				if (tContext->treeTop >= tContext->treeLength - 2)		/* extend tree */
				{
#if USE_TA
					/* Out-of-Memory. */
					if (gcvSTATUS_OK !=
						TA_Resize(os, (void**)&tContext->tree, sizeof(_VGTreeNode) * (tContext->treeLength + tContext->totalPoints * 2), 1))
					{
						gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

						return gcvSTATUS_OUT_OF_MEMORY;
					}
#else
					_VGTreeNode *tempTree;
					OVG_MALLOC(context->os, tempTree, sizeof(_VGTreeNode) * (tContext->treeLength + tContext->totalPoints * 2));
					OVG_MEMSET(tempTree, 0, sizeof(_VGTreeNode) * (tContext->treeLength + tContext->totalPoints * 2));
					OVG_MEMCOPY(tempTree, tContext->tree, sizeof(_VGTreeNode) * tContext->treeLength);
					OVG_SAFE_FREE(context->os, tContext->tree);
					tContext->tree = tempTree;
#endif
					tContext->treeLength = tContext->treeLength + tContext->totalPoints * 2;
				}
				if (leftRegion == 0)
				{	/* topmost region, current region shrink to left, new region created to right */
					tContext->tree[currPosition].leftBranch = tContext->treeTop + 1;
					tContext->tree[currPosition].rightBranch = tContext->treeTop + 2;
					tContext->regionCounter++;
					/* Define right region and add to tree */
					tContext->tree[tContext->treeTop + 2].type = 0;
					tContext->tree[tContext->treeTop + 2].objectIndex = tContext->regionCounter;
					tContext->regions[tContext->regionCounter].upperVertex = currPoint;
					tContext->regions[tContext->regionCounter].lowerVertex = nextPoint;
					tContext->regions[tContext->regionCounter].leftEdge = index;
					tContext->regions[tContext->regionCounter].rightEdge = tContext->regions[currRegion].rightEdge;
					tContext->regions[tContext->regionCounter].treeNode = tContext->treeTop + 2;
					rightRegion = tContext->regionCounter;
					/* Shrink left region and move down one level in tree */
					tContext->tree[tContext->treeTop + 1].type = 0;
					tContext->tree[tContext->treeTop + 1].objectIndex = currRegion;
					tContext->regions[currRegion].rightEdge = index;
					tContext->regions[currRegion].treeNode = tContext->treeTop + 1;
					leftRegion = currRegion;
					/* record the new region */
					if (tContext->edgeHigh[index] != currPoint)
					{	/* could happen when coming out of intersection */
						if (tContext->regions[tContext->regionBelow[currPoint]].upperVertex != currPoint)
						{
							tContext->regionBelow[currPoint] = tContext->regionCounter;
						}
					}
					else if (tContext->regionBelow2[currPoint] == 0)
					{
						tContext->regionBelow2[currPoint] = tContext->regionCounter;
					}
					else if (tContext->regionBelow2[currPoint] == currRegion)
					{
						tContext->regionBelow2[currPoint] = tContext->regionCounter;
						if (tContext->regionBelow3[currPoint][0] == 0)
						{
							tContext->regionBelow3[currPoint][0] = currRegion;
						}
						else
						{
#if USE_TA
							/* Out-of-Memory. */
							if (gcvSTATUS_OK !=
								TA_Resize(os, (void**)&tContext->regionBelow3[currPoint], sizeof(gctINT32) * (tContext->regionBelow3Lengths[currPoint] + 1), 1))
							{
								gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

								return gcvSTATUS_OUT_OF_MEMORY;
							}
#else
							gctINT32 *tempInt;
							OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->regionBelow3Lengths[currPoint] + 1));
							OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->regionBelow3Lengths[currPoint] + 1));
							OVG_MEMCOPY(tempInt, tContext->regionBelow3[currPoint], tContext->regionBelow3Lengths[currPoint] * sizeof(gctINT32));
							OVG_SAFE_FREE(context->os, tContext->regionBelow3[currPoint]);
							tContext->regionBelow3[currPoint] = tempInt;
#endif
							++tContext->regionBelow3Lengths[currPoint];
							tContext->regionBelow3[currPoint][tContext->regionBelow3Lengths[currPoint] - 1] = currRegion;
						}
					}
					else
					{
						if (tContext->regionBelow3[currPoint][0] == 0)
						{
							tContext->regionBelow3[currPoint][0] = tContext->regionCounter;
						}
						else
						{
#if USE_TA
							/* Out-of-Memory. */
							if (gcvSTATUS_OK !=
								TA_Resize(os, (void**)&tContext->regionBelow3[currPoint], sizeof(gctINT32) * (tContext->regionBelow3Lengths[currPoint] + 1), 1))
							{
								gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

								return gcvSTATUS_OUT_OF_MEMORY;
							}
#else
							gctINT32 *tempInt;
							OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->regionBelow3Lengths[currPoint] + 1));
							OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->regionBelow3Lengths[currPoint] + 1));
							OVG_MEMCOPY(tempInt, tContext->regionBelow3[currPoint], sizeof(gctINT32) * tContext->regionBelow3Lengths[currPoint]);
							OVG_SAFE_FREE(context->os, tContext->regionBelow3[currPoint]);
							tContext->regionBelow3[currPoint] = tempInt;
#endif
							++tContext->regionBelow3Lengths[currPoint];

							for (i = tContext->regionBelow3Lengths[currPoint] - 2; i >=0; i--)
							{
								if (currRegion == tContext->regionBelow3[currPoint][i])
								{
									tContext->regionBelow3[currPoint][i + 1] = tContext->regionCounter;
									break;
								}
								else
								{
									tContext->regionBelow3[currPoint][i + 1] = tContext->regionBelow3[currPoint][i];
								}
								if (i == 0)
								{
									tContext->regionBelow3[currPoint][0] = tContext->regionCounter;
								}
							}
						}
					}
					tContext->treeTop += 2;
				}
				else if (direction == 1)
				{	/* right side straight down, region above ended left */
					tContext->tree[currPosition].leftBranch = tContext->treeTop + 1;
					tContext->tree[currPosition].rightBranch = tContext->regions[rightRegion].treeNode;
					tContext->tree[tContext->treeTop + 1].type = 0;
					tContext->tree[tContext->treeTop + 1].objectIndex = currRegion;
					/* Shrink left region */
					tContext->regions[currRegion].rightEdge = index;
					tContext->regions[currRegion].treeNode = tContext->treeTop + 1;
					leftRegion = currRegion;
					/* extend right region */
					tContext->regions[rightRegion].lowerVertex = nextPoint;
					tContext->treeTop += 1;
				}
				else
				{	/* left side straight down */
					tContext->tree[currPosition].leftBranch = tContext->regions[leftRegion].treeNode;
					tContext->tree[currPosition].rightBranch = tContext->treeTop + 1;
					/* Extend left region */
					tContext->regions[leftRegion].lowerVertex = nextPoint;
					tContext->tree[tContext->treeTop + 1].type = 0;
					tContext->tree[tContext->treeTop + 1].objectIndex = currRegion;
					/* shrink right region */
					tContext->regions[currRegion].leftEdge = index;
					tContext->regions[currRegion].treeNode = tContext->treeTop + 1;
					rightRegion = currRegion;
					tContext->treeTop += 1;
				}

				/* Change tree node to edge */
				tContext->tree[currPosition].type = 2;
				tContext->tree[currPosition].objectIndex = index;
			}

			if (nextRegion != 0)
			{
				if (tContext->regions[nextRegion].leftEdge != -1 &&
					tContext->regions[leftRegion].leftEdge != tContext->regions[nextRegion].leftEdge)
				{ /* Left side needs check */
					leftIntersectionStatus = 0;
				}else
				if (tContext->regions[nextRegion].rightEdge != -1 &&
					tContext->regions[rightRegion].rightEdge != tContext->regions[nextRegion].rightEdge)
				{ /* Right side needs check */
					rightIntersectionStatus = 0;
				}
			}

			if (nextRegion == 0)
			{	/* reached end, need to set up region above */
#if USE_TA
				/* Out-of-Memory. */
				if (gcvSTATUS_OK !=
					TA_Resize(os, (void**)&tContext->regionAbove[nextPoint], sizeof(gctINT32) * (tContext->regionAboveLengths[nextPoint] + 1), 1))
				{
					gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

					return gcvSTATUS_OUT_OF_MEMORY;
				}
#else
				gctINT32 *tempInt;
				OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->regionAboveLengths[nextPoint] + 1));
				OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->regionAboveLengths[nextPoint] + 1));
				OVG_MEMCOPY(tempInt, tContext->regionAbove[nextPoint], sizeof(gctINT32) * tContext->regionAboveLengths[nextPoint]);
				OVG_SAFE_FREE(context->os, tContext->regionAbove[nextPoint]);
				tContext->regionAbove[nextPoint] = tempInt;
				tempInt = gcvNULL;
#endif
				++tContext->regionAboveLengths[nextPoint];
				for (i = tContext->regionAboveLengths[nextPoint] - 2; i >=0; i--)
				{
					if (leftRegion == tContext->regionAbove[nextPoint][i])
					{
						tContext->regionAbove[nextPoint][i + 1] = rightRegion;
						break;
					}
					else if (rightRegion == tContext->regionAbove[nextPoint][i])
					{
						tContext->regionAbove[nextPoint][i + 1] = rightRegion;
						tContext->regionAbove[nextPoint][i] = leftRegion;
						break;
					}
					else
					{
						tContext->regionAbove[nextPoint][i + 1] = tContext->regionAbove[nextPoint][i];
					}
				}

				if (tContext->regions[rightRegion].rightEdge != -1 &&
					tContext->edgeHigh[tContext->regions[rightRegion].leftEdge] == tContext->edgeHigh[tContext->regions[rightRegion].rightEdge] &&
					tContext->edgeLow[tContext->regions[rightRegion].leftEdge] == tContext->edgeLow[tContext->regions[rightRegion].rightEdge])
				{
					tContext->regions[rightRegion].degen = gcvTRUE;
				}
				if (tContext->regions[leftRegion].leftEdge != -1 &&
					tContext->edgeHigh[tContext->regions[leftRegion].leftEdge] == tContext->edgeHigh[tContext->regions[leftRegion].rightEdge] &&
					tContext->edgeLow[tContext->regions[leftRegion].leftEdge] == tContext->edgeLow[tContext->regions[leftRegion].rightEdge])
				{
					tContext->regions[leftRegion].degen = gcvTRUE;
				}

			}
			else if (nextDirection == -1 && direction != -1)
			{
				for (i = 0; i < tContext->regionAboveLengths[nextPoint]; i++)
				{
					if (currRegion == tContext->regionAbove[nextPoint][i])
					{
						tContext->regionAbove[nextPoint][i] = rightRegion;
						break;
					}
				}
			}
			else if (nextDirection == 1 && direction == -1)
			{
				for (i = 0; i < tContext->regionAboveLengths[nextPoint]; i++)
				{
					if (currRegion == tContext->regionAbove[nextPoint][i])
					{
						tContext->regionAbove[nextPoint][i] = leftRegion;
						break;
					}
				}
			}
			currPoint = nextPoint;
			if (tContext->errorHandling && (currRegion == nextRegion))
			{
				currRegion = 0;
			}
			else
			{
				currRegion = nextRegion;
			}
			direction = nextDirection;
		}

		index = tContext->totalPoints - 1;
	}while(toContinue);

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
}

/* To break one edge that is not in the tree */
gceSTATUS _BreakOneEdge2(
	_VGContext	*context,
    gctINT32        currRegion,
    gctINT32        theEdge,
    gctINT32        leftPointIndex,
    gctINT32        newEdgeIndex
    )
{
    gctINT32    i = 0;
	_VGTessellationContext	*tContext = &context->tessContext;
    gctINT32    thisLowPoint = tContext->edgeLow[theEdge];
#if USE_TA
	gcoOS os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x currRegion=%d theEdge=%d leftPointIndex=%d newEdgeIndex=%d",
		context, currRegion, theEdge, leftPointIndex, newEdgeIndex);

    tContext->edgeLow[theEdge] = leftPointIndex;
    if (tContext->vertices[thisLowPoint].y == tContext->vertices[tContext->edgeHigh[theEdge]].y)
        tContext->vertices[leftPointIndex].y = tContext->vertices[thisLowPoint].y;
	tContext->edgeLow[newEdgeIndex] = thisLowPoint;
	tContext->edgeHigh[newEdgeIndex] = leftPointIndex;
	tContext->edgeUpDown[newEdgeIndex] = tContext->edgeUpDown[theEdge];

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		TA_Resize(os, (void**)&tContext->edgeIn[leftPointIndex], sizeof(gctINT32) * (tContext->edgeInLengths[leftPointIndex] + 1), 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else

	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeInLengths[leftPointIndex] + 1));
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeInLengths[leftPointIndex] + 1));
	OVG_MEMCOPY(tempInt, tContext->edgeIn[leftPointIndex], sizeof(gctINT32) * tContext->edgeInLengths[leftPointIndex]);
    OVG_SAFE_FREE(context->os, tContext->edgeIn[leftPointIndex]);
	tContext->edgeIn[leftPointIndex] = tempInt;
	tempInt = gcvNULL;
#endif
    ++tContext->edgeInLengths[leftPointIndex];
	tContext->edgeIn[leftPointIndex][tContext->edgeInLengths[leftPointIndex] - 1] = (tContext->edgeUpDown[theEdge] == 1) ? theEdge: newEdgeIndex;

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		TA_Resize(os, (void**)&tContext->edgeOut[leftPointIndex], sizeof(gctINT32) * (tContext->edgeOutLengths[leftPointIndex] + 1), 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeOutLengths[leftPointIndex] + 1));
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeOutLengths[leftPointIndex] + 1));
	OVG_MEMCOPY(tempInt, tContext->edgeOut[leftPointIndex], sizeof(gctINT32) * tContext->edgeOutLengths[leftPointIndex]);
    OVG_SAFE_FREE(context->os, tContext->edgeOut[leftPointIndex]);
	tContext->edgeOut[leftPointIndex] = tempInt;
	tempInt = gcvNULL;
#endif
    ++tContext->edgeOutLengths[leftPointIndex];
	tContext->edgeOut[leftPointIndex][tContext->edgeOutLengths[leftPointIndex] - 1] = (tContext->edgeUpDown[theEdge] == 1) ? newEdgeIndex : theEdge;
	if (tContext->edgeUpDown[theEdge] == 1)
	{
		for (i = 0; i < tContext->edgeInLengths[thisLowPoint]; i++)
		{
			if (tContext->edgeIn[thisLowPoint][i] == theEdge)
			{
				tContext->edgeIn[thisLowPoint][i] = newEdgeIndex;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < tContext->edgeOutLengths[thisLowPoint]; i++)
		{
			if (tContext->edgeOut[thisLowPoint][i] == theEdge)
			{
				tContext->edgeOut[thisLowPoint][i] = newEdgeIndex;
				break;
			}
		}
	}
	if (_PointHigh(tContext, leftPointIndex, tContext->edgeHigh[theEdge]))
	{
        tContext->vertices[leftPointIndex].y = tContext->vertices[tContext->edgeHigh[theEdge]].y;
        if (tContext->vertices[leftPointIndex].x < tContext->vertices[tContext->edgeHigh[theEdge]].x)
            tContext->vertices[leftPointIndex].x = tContext->vertices[tContext->edgeHigh[theEdge]].x;
	}
	if (_PointHigh(tContext, thisLowPoint, leftPointIndex))
	{
		tContext->edgeHigh[newEdgeIndex] = thisLowPoint;
		tContext->edgeLow[newEdgeIndex] = leftPointIndex;
		tContext->edgeUpDown[newEdgeIndex] = -tContext->edgeUpDown[newEdgeIndex];
	}

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
}

/* Find a region on the other side of an edge */
gctINT32 _FindOtherRegion(
	_VGTessellationContext	*tContext,
    gctINT32        thisRegion,
    gctINT32        thisPoint,
    gctINT32        direction
    )
{
    gctINT32 i;
	gctINT32 theEdge = 0, topPoint = 0;
	gctINT32 otherRegion = -1;
	gctINT32 below;

	gcmHEADER_ARG("tContext=0x%x thisRegion=%d thisPoint=%d direction=%d", tContext, thisRegion, thisPoint, direction);

	if (direction == -1)
	{	/* left */
		theEdge = tContext->regions[thisRegion].leftEdge;
		topPoint = tContext->edgeHigh[theEdge];
		if (tContext->regions[tContext->regionBelow[topPoint]].rightEdge == theEdge)
		{
			otherRegion = tContext->regionBelow[topPoint];
		}
		else
		{
			for (i = 0; i < tContext->regionBelow3Lengths[topPoint]; i++)
			{
				if (tContext->regions[tContext->regionBelow3[topPoint][i]].rightEdge == theEdge)
				{
					otherRegion = tContext->regionBelow3[topPoint][i];
					break;
				}
			}
		}
		while (_PointHigh(tContext, tContext->regions[otherRegion].lowerVertex, thisPoint))
		{
			below = tContext->regionBelow2[tContext->regions[otherRegion].lowerVertex];
			otherRegion = (below == 0) ? tContext->regionBelow[tContext->regions[otherRegion].lowerVertex] : below;
		}
	}
	else
	{
		theEdge = tContext->regions[thisRegion].rightEdge;
		topPoint = tContext->edgeHigh[theEdge];
		if (tContext->regions[tContext->regionBelow2[topPoint]].leftEdge == theEdge)
		{
			otherRegion = tContext->regionBelow2[topPoint];
		}
		else
		{
			for (i = 0; i < tContext->regionBelow3Lengths[topPoint]; i++)
			{
				if (tContext->regions[tContext->regionBelow3[topPoint][i]].leftEdge == theEdge)
				{
					otherRegion = tContext->regionBelow3[topPoint][i];
					break;
				}
			}
		}
		while (_PointHigh(tContext, tContext->regions[otherRegion].lowerVertex, thisPoint))
		{
			otherRegion = tContext->regionBelow[tContext->regions[otherRegion].lowerVertex];
		}
	}

	gcmFOOTER_ARG("return=%d", otherRegion);

	return otherRegion;
}

/* List all regions involved from left to right */
gctINT32 *_FindOtherRegions(
	_VGContext	*context,
    gctINT32 thisRegion,
    gctINT32 thisPoint,
    gctINT32 direction,
    gctINT32 above,
    gctINT32 *arrayLength
    )
{
    gctINT32    i;
	gctINT32    theEdge = 0, topPoint = 0;
	gctINT32    *otherRegions = gcvNULL, otherRegionsLength = 0;
	_VGTessellationContext	*tContext = &context->tessContext;

	gctINT32 below;
	gctINT32 startRegion, endRegion;

	gcmHEADER_ARG("context=0x%x thisRegion=%d thisPoint=%d direction=%d above=%d arrayLength=0x%x",
		context, thisRegion, thisPoint, direction, above, arrayLength);

	if (direction == -1)
	{	/* left */
		theEdge = tContext->regions[thisRegion].leftEdge;
		topPoint = tContext->edgeHigh[theEdge];
		if (tContext->regions[tContext->regionBelow[topPoint]].rightEdge == theEdge)
		{
#if LOCAL_MEM_OPTIM
			_AllocateInt(tContext->IntPool, &otherRegions, 2);
#else
			OVG_MALLOC(context->os, otherRegions, sizeof(gctINT32) * 2);
			/* Out-of-Memory. */
			if (otherRegions == gcvNULL)
			{
				gcmFOOTER_ARG("return=0x%x", gcvNULL);

				return gcvNULL;
			}
#endif
			otherRegions[0] = tContext->regionBelow[topPoint];
			otherRegions[1] = thisRegion;
            otherRegionsLength = 2;
		}
		else
		{
			startRegion = -1;
			endRegion = -1;
			for (i = 0; i < tContext->regionBelow3Lengths[topPoint]; i++)
			{
				if (!tContext->regions[tContext->regionBelow3[topPoint][i]].degen)
				{
					startRegion = i;
				}
				if (tContext->regions[tContext->regionBelow3[topPoint][i]].rightEdge == theEdge)
				{
					endRegion = i;
					break;
				}
			}
            otherRegionsLength = endRegion - startRegion + 2;
#if LOCAL_MEM_OPTIM
			_AllocateInt(tContext->IntPool, &otherRegions, otherRegionsLength);
#else
			OVG_MALLOC(context->os, otherRegions, sizeof(gctINT32) * otherRegionsLength);
			/* Out-of-Memory. */
			if (otherRegions == gcvNULL)
			{
				gcmFOOTER_ARG("return=0x%x", gcvNULL);

				return gcvNULL;
			}
#endif
            OVG_MEMSET(otherRegions, 0, sizeof(gctINT32) * otherRegionsLength);
			otherRegions[0] = (startRegion == -1) ? tContext->regionBelow[topPoint] : tContext->regionBelow3[topPoint][startRegion];
			for (i = 1; i < otherRegionsLength; i++)
			{
				otherRegions[i] = tContext->regionBelow3[topPoint][startRegion + i];
			}
			otherRegions[otherRegionsLength - 1] = thisRegion;
		}
		while (_PointHigh(tContext, tContext->regions[otherRegions[0]].lowerVertex, thisPoint))
		{
			below = tContext->regionBelow2[tContext->regions[otherRegions[0]].lowerVertex];
			otherRegions[0] = (below == 0) ? tContext->regionBelow[tContext->regions[otherRegions[0]].lowerVertex] : below;
		}
	}
	else
	{
		theEdge = tContext->regions[thisRegion].rightEdge;
		topPoint = tContext->edgeHigh[theEdge];
		if (tContext->regions[tContext->regionBelow2[topPoint]].leftEdge == theEdge)
		{
#if LOCAL_MEM_OPTIM
			_AllocateInt(tContext->IntPool, &otherRegions, 2);
#else
			OVG_MALLOC(context->os, otherRegions, sizeof(gctINT32) * 2);
			/* Out-of-Memory. */
			if (otherRegions == gcvNULL)
			{
				gcmFOOTER_ARG("return=0x%x", gcvNULL);

				return gcvNULL;
			}
#endif
            otherRegionsLength = 2;
			otherRegions[0] = thisRegion;
			otherRegions[1] = tContext->regionBelow2[topPoint];
		}
		else
		{
			startRegion = -1;
			endRegion = tContext->regionBelow3Lengths[topPoint];
			for (i = 0; i < tContext->regionBelow3Lengths[topPoint]; i++)
			{
				if (tContext->regions[tContext->regionBelow3[topPoint][i]].leftEdge == theEdge)
				{
					startRegion = i;
				}
				if (startRegion > -1 && !tContext->regions[tContext->regionBelow3[topPoint][i]].degen)
				{
					endRegion = i;
					break;
				}
			}
            otherRegionsLength = endRegion-startRegion+2;
#if LOCAL_MEM_OPTIM
			_AllocateInt(tContext->IntPool, &otherRegions, otherRegionsLength);
#else
			OVG_MALLOC(context->os, otherRegions, sizeof(gctINT32) * otherRegionsLength);
			/* Out-of-Memory. */
			if (otherRegions == gcvNULL)
			{
				gcmFOOTER_ARG("return=0x%x", gcvNULL);

				return gcvNULL;
			}
#endif
            OVG_MEMSET(otherRegions, 0, sizeof(gctINT32) * otherRegionsLength);
			otherRegions[0] = thisRegion;
			for (i = 1; i < otherRegionsLength - 1; i++)
			{
				otherRegions[i] = tContext->regionBelow3[topPoint][startRegion + i - 1];
			}
			otherRegions[otherRegionsLength - 1] = (endRegion == tContext->regionBelow3Lengths[topPoint]) ?
                                                   tContext->regionBelow2[topPoint] : tContext->regionBelow3[topPoint][endRegion];
		}
		while (tContext->regions[otherRegions[otherRegionsLength - 1]].lowerVertex != above &&
               _PointHigh(tContext, tContext->regions[otherRegions[otherRegionsLength - 1]].lowerVertex, thisPoint))
		{
			otherRegions[otherRegionsLength - 1] = tContext->regionBelow[tContext->regions[otherRegions[otherRegionsLength - 1]].lowerVertex];
		}
	}

    *arrayLength = otherRegionsLength;

	gcmFOOTER_ARG("return=0x%x", otherRegions);

	return otherRegions;
}

/* Break two intersecting edges */
gceSTATUS _BreakEdge(
	_VGContext	*context,
    gctINT32        edgeToBreak,
    gctINT32        newEdgeIndex,
    gctINT32        leftRegion,
    gctINT32        rightRegion,
    gctINT32        leftPoint,
    gctINT32        rightPoint,
    gctINT32        breakType
    )
{
    gctINT32 i = 0;
	gctINT32 lowerLeft = 0, lowerRight = 0;
	_VGTessellationContext	*tContext = &context->tessContext;
	gctINT32 lowPoint = tContext->edgeLow[edgeToBreak];
	gctINT32 moveDown;
#if USE_TA
	gcoOS os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x edgeToBreak=%d newEdgeIndex=%d leftRegion=%d rightRegion=%d "
		"leftPoint=%d rightPoint=%d breakType=%d",
		context, edgeToBreak, newEdgeIndex, leftRegion, rightRegion, leftPoint, rightPoint, breakType);

	if (breakType == 0)
	{
		tContext->edgeLow[edgeToBreak] = leftPoint;
		tContext->edgeHigh[newEdgeIndex] = leftPoint;
	}
	else
	{	/* new point on left */
		tContext->edgeLow[edgeToBreak] = rightPoint;
		tContext->edgeHigh[newEdgeIndex] = rightPoint;
	}
	tContext->edgeLow[newEdgeIndex] = lowPoint;
	tContext->edgeUpDown[newEdgeIndex] = tContext->edgeUpDown[edgeToBreak];

	if (breakType == 0)
	{
		if (tContext->edgeInLengths[leftPoint] > 1 || tContext->edgeIn[leftPoint][0] > -1)
		{
#if USE_TA
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				TA_Resize(os, (void**)&tContext->edgeIn[leftPoint], sizeof(gctINT32) * (tContext->edgeInLengths[leftPoint] + 1), 1))
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
#else
			OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeInLengths[leftPoint] + 1));
            OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeInLengths[leftPoint] + 1));
            OVG_MEMCOPY(tempInt, tContext->edgeIn[leftPoint], sizeof(gctINT32) * tContext->edgeInLengths[leftPoint]);
            OVG_SAFE_FREE(context->os, tContext->edgeIn[leftPoint]);
			tContext->edgeIn[leftPoint] = tempInt;
			tempInt = gcvNULL;
#endif
            ++tContext->edgeInLengths[leftPoint];
		}
		tContext->edgeIn[leftPoint][tContext->edgeInLengths[leftPoint] - 1] = (tContext->edgeUpDown[edgeToBreak] == 1) ? edgeToBreak: newEdgeIndex;
		if (tContext->edgeOutLengths[leftPoint] > 1 || tContext->edgeOut[leftPoint][0] > -1)
		{
#if USE_TA
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				TA_Resize(os, (void**)&tContext->edgeOut[leftPoint], sizeof(gctINT32) * (tContext->edgeOutLengths[leftPoint] + 1), 1))
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
#else
			OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeOutLengths[leftPoint] + 1));
            OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeOutLengths[leftPoint] + 1));
            OVG_MEMCOPY(tempInt, tContext->edgeOut[leftPoint], sizeof(gctINT32) * tContext->edgeOutLengths[leftPoint]);
            OVG_SAFE_FREE(context->os, tContext->edgeOut[leftPoint]);
			tContext->edgeOut[leftPoint] = tempInt;
			tempInt = gcvNULL;
#endif
            ++tContext->edgeOutLengths[leftPoint];
		}
		tContext->edgeOut[leftPoint][tContext->edgeOutLengths[leftPoint] - 1] = (tContext->edgeUpDown[edgeToBreak] == 1) ? newEdgeIndex : edgeToBreak;
	}
	else
	{
		if (tContext->edgeInLengths[rightPoint] > 1 || tContext->edgeIn[rightPoint][0] > -1)
		{
#if USE_TA
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				TA_Resize(os, (void**)&tContext->edgeIn[rightPoint], sizeof(gctINT32) * (tContext->edgeInLengths[rightPoint] + 1), 1))
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
#else
			OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeInLengths[rightPoint] + 1));
            OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeInLengths[rightPoint] + 1));
            OVG_MEMCOPY(tempInt, tContext->edgeIn[rightPoint], sizeof(gctINT32) * tContext->edgeInLengths[rightPoint]);
            OVG_SAFE_FREE(context->os, tContext->edgeIn[rightPoint]);
			tContext->edgeIn[rightPoint] = tempInt;
			tempInt = gcvNULL;
#endif
            ++tContext->edgeInLengths[rightPoint];
		}
		tContext->edgeIn[rightPoint][tContext->edgeInLengths[rightPoint]  -1] = (tContext->edgeUpDown[edgeToBreak] == 1) ? edgeToBreak: newEdgeIndex;
		if (tContext->edgeOutLengths[rightPoint] > 1 || tContext->edgeOut[rightPoint][0] > -1)
		{
#if USE_TA
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				TA_Resize(os, (void**)&tContext->edgeOut[rightPoint], sizeof(gctINT32) * (tContext->edgeOutLengths[rightPoint] + 1), 1))
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
#else
			OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeOutLengths[rightPoint] + 1));
            OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeOutLengths[rightPoint] + 1));
            OVG_MEMCOPY(tempInt, tContext->edgeOut[rightPoint], sizeof(gctINT32) * tContext->edgeOutLengths[rightPoint]);
            OVG_SAFE_FREE(context->os, tContext->edgeOut[rightPoint]);
			tContext->edgeOut[rightPoint] = tempInt;
			tempInt = gcvNULL;
#endif
            ++tContext->edgeOutLengths[rightPoint];
		}
		tContext->edgeOut[rightPoint][tContext->edgeOutLengths[rightPoint] - 1] = (tContext->edgeUpDown[edgeToBreak] == 1) ? newEdgeIndex : edgeToBreak;
	}

    if (tContext->edgeUpDown[edgeToBreak] == 1)
	{
		for (i = 0; i < tContext->edgeInLengths[lowPoint]; i++)
		{
			if (tContext->edgeIn[lowPoint][i] == edgeToBreak)
			{
				tContext->edgeIn[lowPoint][i] = newEdgeIndex;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < tContext->edgeOutLengths[lowPoint]; i++)
		{
			if (tContext->edgeOut[lowPoint][i] == edgeToBreak)
			{
				tContext->edgeOut[lowPoint][i] = newEdgeIndex;
				break;
			}
		}
	}

	if (breakType == 0)
	{
		tContext->edgeIn[rightPoint][0] = -1;
		tContext->edgeOut[rightPoint][0] = -1;
		tContext->tree[tContext->pointsTreePosition[rightPoint]].objectIndex = leftPoint;
		tContext->pointsTreePosition[rightPoint] = 0;
		if (_PointHigh(tContext, leftPoint, tContext->edgeHigh[edgeToBreak]))
		{
            tContext->vertices[leftPoint].y = tContext->vertices[tContext->edgeHigh[edgeToBreak]].y;
            if (tContext->vertices[leftPoint].x < tContext->vertices[tContext->edgeHigh[edgeToBreak]].x)
                tContext->vertices[leftPoint].x = tContext->vertices[tContext->edgeHigh[edgeToBreak]].x;
		}
		if (_PointHigh(tContext, lowPoint, leftPoint))
		{
            tContext->vertices[leftPoint].y = tContext->vertices[lowPoint].y;
            if (tContext->vertices[leftPoint].x < tContext->vertices[lowPoint].x)
                tContext->vertices[leftPoint].x = tContext->vertices[lowPoint].x;
		}

		lowerRight = tContext->regionBelow[rightPoint];
		if (tContext->regionBelow2[leftPoint] == 0)
		{
			lowerLeft = tContext->regionBelow[leftPoint];
		}
		else
		{
			lowerLeft = tContext->regionBelow2[leftPoint];
			if (tContext->regionBelow3[leftPoint][0] == 0)
			{
				tContext->regionBelow3[leftPoint][0] = lowerLeft;
			}
			else
			{
#if USE_TA
				/* Out-of-Memory. */
				if (gcvSTATUS_OK !=
					TA_Resize(os, (void**)&tContext->regionBelow3[leftPoint], sizeof(gctINT32) * (tContext->regionBelow3Lengths[leftPoint] + 1), 1))
				{
					gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

					return gcvSTATUS_OUT_OF_MEMORY;
				}
#else
				OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->regionBelow3Lengths[leftPoint] + 1));
                OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->regionBelow3Lengths[leftPoint] + 1));
                OVG_MEMCOPY(tempInt, tContext->regionBelow3[leftPoint], sizeof(gctINT32) * tContext->regionBelow3Lengths[leftPoint]);
                OVG_SAFE_FREE(context->os, tContext->regionBelow3[leftPoint]);
				tContext->regionBelow3[leftPoint] = tempInt;
#endif
				tContext->regionBelow3[leftPoint][tContext->regionBelow3Lengths[leftPoint] - 1] = lowerLeft;
                ++tContext->regionBelow3Lengths[leftPoint];
			}
		}
		tContext->regionBelow2[leftPoint] = lowerRight;
		tContext->regionBelow[rightPoint] = 0;
		tContext->regions[rightRegion].lowerVertex = leftPoint;
		tContext->regions[lowerRight].upperVertex = leftPoint;
#if USE_TA
		/* Out-of-Memory. */
		if (gcvSTATUS_OK !=
			TA_Resize(os, (void**)&tContext->regionAbove[leftPoint], sizeof(gctINT32) * (tContext->regionAboveLengths[leftPoint] + 1), 1))
		{
			gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

			return gcvSTATUS_OUT_OF_MEMORY;
		}
#else
		OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->regionAboveLengths[leftPoint] + 1));
        OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->regionAboveLengths[leftPoint] + 1));
        OVG_MEMCOPY(tempInt, tContext->regionAbove[leftPoint], sizeof(gctINT32) * tContext->regionAboveLengths[leftPoint]);
        OVG_SAFE_FREE(context->os, tContext->regionAbove[leftPoint]);
		tContext->regionAbove[leftPoint] = tempInt;
#endif
        ++tContext->regionAboveLengths[leftPoint];
		tContext->regionAbove[leftPoint][tContext->regionAboveLengths[leftPoint] - 1] = rightRegion;
		tContext->regionAbove[rightPoint][0] = 0;
	}
	else
	{
		tContext->edgeIn[leftPoint][0] = -1;
		tContext->edgeOut[leftPoint][0] = -1;
		tContext->tree[tContext->pointsTreePosition[leftPoint]].objectIndex = rightPoint;
		tContext->pointsTreePosition[leftPoint] = 0;
		if (_PointHigh(tContext, rightPoint, tContext->edgeHigh[edgeToBreak]))
		{
            tContext->vertices[rightPoint].y = tContext->vertices[tContext->edgeHigh[edgeToBreak]].y;
            if (tContext->vertices[rightPoint].x < tContext->vertices[tContext->edgeHigh[edgeToBreak]].x)
                tContext->vertices[rightPoint].x = tContext->vertices[tContext->edgeHigh[edgeToBreak]].x;
		}
		if (_PointHigh(tContext, lowPoint, rightPoint))
		{
            tContext->vertices[rightPoint].y = tContext->vertices[lowPoint].y;
            if (tContext->vertices[rightPoint].x < tContext->vertices[lowPoint].x)
                tContext->vertices[rightPoint].x = tContext->vertices[lowPoint].x;
		}

		lowerRight = tContext->regionBelow[rightPoint];
		lowerLeft = tContext->regionBelow[leftPoint];
		if (tContext->regionBelow2[rightPoint] == 0)
		{
			tContext->regionBelow2[rightPoint] = lowerRight;
		}
		else if (tContext->regionBelow3[rightPoint][0] == 0)
		{
			tContext->regionBelow3[rightPoint][0] = lowerRight;
		}
		else
		{
#if USE_TA
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				TA_Resize(os, (void**)&tContext->regionBelow3[rightPoint], sizeof(gctINT32) * (tContext->regionBelow3Lengths[rightPoint] + 1), 1))
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
#else
			OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->regionBelow3Lengths[rightPoint] + 1));
            OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->regionBelow3Lengths[rightPoint] + 1));
            OVG_MEMCOPY(tempInt, tContext->regionBelow3[rightPoint], sizeof(gctINT32) * tContext->regionBelow3Lengths[rightPoint]);
            OVG_SAFE_FREE(context->os, tContext->regionBelow3[rightPoint]);
			tContext->regionBelow3[rightPoint] = tempInt;
#endif
            ++tContext->regionBelow3Lengths[rightPoint];
			tContext->regionBelow3[rightPoint][0] = lowerRight;
		}
		tContext->regionBelow[rightPoint] = lowerLeft;
		tContext->regionBelow[leftPoint] = 0;
		tContext->regions[leftRegion].lowerVertex = rightPoint;
		tContext->regions[lowerLeft].upperVertex = rightPoint;
#if USE_TA
		/* Out-of-Memory. */
		if (gcvSTATUS_OK !=
			TA_Resize(os, (void**)&tContext->regionAbove[rightPoint], sizeof(gctINT32) * (tContext->regionAboveLengths[rightPoint] + 1), 1))
		{
			gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

			return gcvSTATUS_OUT_OF_MEMORY;
		}
#else
		OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->regionAboveLengths[rightPoint] + 1));
        OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->regionAboveLengths[rightPoint] + 1));
        OVG_MEMCOPY(tempInt, tContext->regionAbove[rightPoint], sizeof(gctINT32) * tContext->regionAboveLengths[rightPoint]);
        OVG_SAFE_FREE(context->os, tContext->regionAbove[rightPoint]);
		tContext->regionAbove[rightPoint] = tempInt;
#endif
        ++tContext->regionAboveLengths[rightPoint];
		tContext->regionAbove[rightPoint][0] = leftRegion;
		tContext->regionAbove[leftPoint][0] = 0;
	}

    while (tContext->regions[lowerLeft].rightEdge == edgeToBreak)
	{
        moveDown = 0;
		tContext->regions[lowerLeft].rightEdge = newEdgeIndex;
		moveDown = tContext->regionBelow2[tContext->regions[lowerLeft].lowerVertex];
		lowerLeft = (moveDown == 0) ? tContext->regionBelow[tContext->regions[lowerLeft].lowerVertex] : moveDown;
	}
	while (tContext->regions[lowerRight].leftEdge == edgeToBreak)
	{
		tContext->regions[lowerRight].leftEdge = newEdgeIndex;
		lowerRight = tContext->regionBelow[tContext->regions[lowerRight].lowerVertex];
	}

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
}

/* When 2 points sharing the same location, give all edges to one point. Here leftPoint is a point already added
	* to the tree, right point is not, and will never be added		*/
gceSTATUS _RearrangeEdges(
	_VGContext	*context,
    gctINT32        leftPoint,
    gctINT32        rightPoint,
    gctINT32        newPoint
    )
{
	_VGTessellationContext	*tContext = &context->tessContext;

#if USE_TA
	gcoOS os = context->os;

	gcmHEADER_ARG("context=0x%x leftPoint=%d rightPoint=%d newPoint=%d",
		context, leftPoint, rightPoint, newPoint);

	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		TA_Resize(os, (void**)&tContext->edgeIn[leftPoint], sizeof(gctINT32) * (tContext->edgeInLengths[leftPoint] + 1), 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	gctINT32 *tempInt;
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeInLengths[leftPoint] + 1));
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeInLengths[leftPoint] + 1));
	OVG_MEMCOPY(tempInt, tContext->edgeIn[leftPoint], sizeof(gctINT32) * tContext->edgeInLengths[leftPoint]);
	OVG_SAFE_FREE(context->os, tContext->edgeIn[leftPoint]);
    tContext->edgeIn[leftPoint] = tempInt;
	tempInt = gcvNULL;
#endif
    ++tContext->edgeInLengths[leftPoint];
	tContext->edgeIn[leftPoint][tContext->edgeInLengths[leftPoint] - 1] = tContext->edgeIn[rightPoint][0];

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		TA_Resize(os, (void**)&tContext->edgeOut[leftPoint], sizeof(gctINT32) * (tContext->edgeOutLengths[leftPoint] + 1), 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * (tContext->edgeOutLengths[leftPoint] + 1));
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * (tContext->edgeOutLengths[leftPoint] + 1));
	OVG_MEMCOPY(tempInt, tContext->edgeOut[leftPoint], sizeof(gctINT32) * tContext->edgeOutLengths[leftPoint]);
    OVG_SAFE_FREE(context->os, tContext->edgeOut[leftPoint]);
	tContext->edgeOut[leftPoint] = tempInt;
	tempInt = gcvNULL;
#endif
    ++tContext->edgeOutLengths[leftPoint];
	tContext->edgeOut[leftPoint][tContext->edgeOutLengths[leftPoint] - 1] = tContext->edgeOut[rightPoint][0];
	if (tContext->edgeUpDown[tContext->edgeIn[rightPoint][0]] == 1)
	{
		tContext->edgeLow[tContext->edgeIn[rightPoint][0]] = leftPoint;
	}
	else
	{
		tContext->edgeHigh[tContext->edgeIn[rightPoint][0]] = leftPoint;
	}
	if (tContext->edgeUpDown[tContext->edgeOut[rightPoint][0]] == 1)
	{
		tContext->edgeHigh[tContext->edgeOut[rightPoint][0]] = leftPoint;
	}
	else
	{
		tContext->edgeLow[tContext->edgeOut[rightPoint][0]] = leftPoint;
	}
	tContext->edgeIn[rightPoint][0] = -1;
	tContext->edgeOut[rightPoint][0] = -1;

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
}


gceSTATUS _ResizeArrays(
	_VGContext	*context,
    gctINT32        newLength
    )
{
    gctINT32        i = 0;

	_VGTessellationContext	*tContext = &context->tessContext;
    _VGVector2    *tempVertices = gcvNULL;
#if USE_TA
	gcoOS	os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x newLength=%d", context, newLength);

	OVG_MALLOC(context->os, tempVertices, sizeof(_VGVector2) * newLength);
	/* Out-of-Memory. */
	if (tempVertices == gcvNULL)
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
    OVG_MEMSET(tempVertices, 0, sizeof(_VGVector2) * newLength);
	OVG_MEMCOPY(tempVertices, tContext->vertices, sizeof(_VGVector2) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->vertices);
	tContext->vertices = tempVertices;
    tContext->verticesLength = newLength;

#if USE_TA
	/* Out-of-Memory. */
	if ((gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->edgeHigh, sizeof(gctINT32) * newLength, 1))	||
		(gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->edgeLow, sizeof(gctINT32) * newLength, 1))	||
		(gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->edgeUpDown, sizeof(gctINT32) * newLength, 1))	||
		(gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->pointsTreePosition, sizeof(gctINT32) * newLength, 1))	||
		(gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->regionBelow, sizeof(gctINT32) * newLength, 1))	||
		(gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->regionBelow2, sizeof(gctINT32) * newLength, 1))	||
#else
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
	OVG_MEMCOPY(tempInt, tContext->edgeHigh, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->edgeHigh);
	tContext->edgeHigh = tempInt;

	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
	OVG_MEMCOPY(tempInt, tContext->edgeLow, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->edgeLow);
	tContext->edgeLow = tempInt;

	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
	OVG_MEMCOPY(tempInt, tContext->edgeUpDown, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->edgeUpDown);
	tContext->edgeUpDown = tempInt;

	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
	OVG_MEMCOPY(tempInt, tContext->pointsTreePosition, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->pointsTreePosition);
	tContext->pointsTreePosition = tempInt;

	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
	OVG_MEMCOPY(tempInt, tContext->regionBelow, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->regionBelow);
	tContext->regionBelow = tempInt;

	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
	OVG_MEMCOPY(tempInt, tContext->regionBelow2, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->regionBelow2);
	tContext->regionBelow2 = tempInt;
#endif

#if USE_TA
		(gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->regionBelow3, sizeof(gctINT32*) * newLength, 2))	||
		(gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->regionBelow3Lengths, sizeof(gctINT32) * newLength, 1)))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	OVG_MALLOC(context->os, tempIntInt, sizeof(gctINT32*) * newLength);
    OVG_MEMSET(tempIntInt, 0, sizeof(gctINT32*) * newLength);
    OVG_MEMCOPY(tempIntInt, tContext->regionBelow3, sizeof(gctINT32*) * tContext->totalPoints);
    for (i = tContext->totalPoints; i < tContext->regionBelow3Length; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->regionBelow3[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->regionBelow3);
	tContext->regionBelow3 = tempIntInt;
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
    OVG_MEMCOPY(tempInt, tContext->regionBelow3Lengths, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->regionBelow3Lengths);
    tContext->regionBelow3Lengths = tempInt;
#endif
    tContext->regionBelow3Length = newLength;
	for (i = tContext->totalPoints; i < newLength; i++)
	{
#if USE_TA
#else
		OVG_MALLOC(context->os, tContext->regionBelow3[i], sizeof(gctINT32));
#endif
        tContext->regionBelow3[i][0] = 0;
        tContext->regionBelow3Lengths[i] = 1;
	}

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->regionAbove, sizeof(gctINT32*) * newLength, 2) ||
		gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->regionAboveLengths, sizeof(gctINT32) * newLength, 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	OVG_MALLOC(context->os, tempIntInt, sizeof(gctINT32*) * newLength);
    OVG_MEMSET(tempIntInt, 0, sizeof(gctINT32*) * newLength);
	OVG_MEMCOPY(tempIntInt, tContext->regionAbove, sizeof(gctINT32*) * tContext->totalPoints);
    for (i = tContext->totalPoints; i < tContext->regionAboveLength; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->regionAbove[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->regionAbove);
	tContext->regionAbove = tempIntInt;
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
    OVG_MEMCOPY(tempInt, tContext->regionAboveLengths, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->regionAboveLengths);
    tContext->regionAboveLengths = tempInt;
#endif
    tContext->regionAboveLength = newLength;
	for (i = tContext->totalPoints; i < newLength; i++)
	{
#if !USE_TA
		OVG_MALLOC(context->os, tContext->regionAbove[i], sizeof(gctINT32));
#endif
        tContext->regionAbove[i][0] = 0;
        tContext->regionAboveLengths[i] = 1;
	}

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->edgeIn, sizeof(gctINT32*) * newLength, 2)	||
		gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->edgeInLengths, sizeof(gctINT32*) * newLength, 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	OVG_MALLOC(context->os, tempIntInt, sizeof(gctINT32*) * newLength);
    OVG_MEMSET(tempIntInt, 0, sizeof(gctINT32*) * newLength);
	OVG_MEMCOPY(tempIntInt, tContext->edgeIn, sizeof(gctINT32*) * tContext->totalPoints);
    for (i = tContext->totalPoints; i < tContext->edgeInLength; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->edgeIn[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->edgeIn);
	tContext->edgeIn = tempIntInt;
	tempIntInt = gcvNULL;
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
    OVG_MEMCOPY(tempInt, tContext->edgeInLengths, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->edgeInLengths);
    tContext->edgeInLengths = tempInt;
#endif
    tContext->edgeInLength = newLength;
	for (i = tContext->totalPoints; i < newLength; i++)
	{
#if USE_TA
#else
		OVG_MALLOC(context->os, tContext->edgeIn[i], sizeof(gctINT32));
#endif
        tContext->edgeIn[i][0] = 0;
        tContext->edgeInLengths[i] = 1;
	}

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->edgeOut, sizeof(gctINT32*) * newLength, 2)	||
		gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->edgeOutLengths, sizeof(gctINT32) * newLength, 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	OVG_MALLOC(context->os, tempIntInt, sizeof(gctINT32*) * newLength);
    OVG_MEMSET(tempIntInt, 0, sizeof(gctINT32*) * newLength);
	OVG_MEMCOPY(tempIntInt, tContext->edgeOut, sizeof(gctINT32*) * tContext->totalPoints);
    for (i = tContext->totalPoints; i < tContext->edgeOutLength; i++)
    {
        OVG_SAFE_FREE(context->os, tContext->edgeOut[i]);
    }
    OVG_SAFE_FREE(context->os, tContext->edgeOut);
	tContext->edgeOut = tempIntInt;
	tempIntInt = gcvNULL;
	OVG_MALLOC(context->os, tempInt, sizeof(gctINT32) * newLength);
    OVG_MEMSET(tempInt, 0, sizeof(gctINT32) * newLength);
    OVG_MEMCOPY(tempInt, tContext->edgeOutLengths, sizeof(gctINT32) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->edgeOutLengths);
    tContext->edgeOutLengths = tempInt;
#endif
    tContext->edgeOutLength = newLength;
	for (i = tContext->totalPoints; i < newLength; i++)
	{
#if USE_TA
#else
		OVG_MALLOC(context->os, tContext->edgeOut[i], sizeof(gctINT32));
#endif
        tContext->edgeOut[i][0] = 0;
        tContext->edgeOutLengths[i] = 1;
	}

#if USE_TA
	/* Out-of-Memory. */
	if (gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->pointsAdded, sizeof(gctBOOL) * newLength, 1)	||
		gcvSTATUS_OK != TA_Resize(os, (void**)&tContext->regions, sizeof(_VGTrapezoid) * (newLength * 2 + 1), 1))
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
#else
	OVG_MALLOC(context->os, tempBool, sizeof(gctBOOL) * newLength);
    OVG_MEMSET(tempBool, gcvFALSE, sizeof(gctBOOL) * newLength);
	OVG_MEMCOPY(tempBool, tContext->pointsAdded, sizeof(gctBOOL) * tContext->totalPoints);
    OVG_SAFE_FREE(context->os, tContext->pointsAdded);
	tContext->pointsAdded = tempBool;

	OVG_MALLOC(context->os, tempRegions, sizeof(_VGTrapezoid) * (newLength * 2 + 1));
    OVG_MEMSET(tempRegions, 0, sizeof(_VGTrapezoid) * (newLength * 2 + 1));
	OVG_MEMCOPY(tempRegions, tContext->regions, sizeof(_VGTrapezoid) * (tContext->totalPoints * 2 + 1));
    OVG_SAFE_FREE(context->os, tContext->regions);
	tContext->regions = tempRegions;
#endif
    tContext->regionsLength = newLength * 2 + 1;

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
}

/* We assume top of edge1 is to the right of edge2 */
gctINT32 _CheckIntersection(
	_VGTessellationContext	*tContext,
    gctINT32        edge1,
    gctINT32        edge2,
    _VGVector2		*intersection
    )
{
    _VGTesstype x1, x2, y1, y2, x, y;
    _VGTesstype z1, z2, dx1, dx2, dy1, dy2;
	_VGTessHighType xden, xnum; /* 64 bit for fix point */
    gctINT32 highBottom, lowTop;

	/* 5 = left edge right vertex, 6 = left vertex right edge */
	if (tContext->edgeLow[edge1] == tContext->edgeLow[edge2])
	{
        *intersection = tContext->vertices[tContext->edgeLow[edge1]];

		return 3;
	}
	else
	{
		gctBOOL edge1High = _PointHigh(tContext, tContext->edgeLow[edge1], tContext->edgeLow[edge2]);
		if (edge1High)
		{
			gctINT32 pos = _PointLeftRight(tContext, tContext->edgeLow[edge1], edge2);
			if (pos == 0)
			{
                *intersection = tContext->vertices[tContext->edgeLow[edge1]];

                return (tContext->vertices[tContext->edgeLow[edge1]].x == tContext->vertices[tContext->edgeLow[edge2]].x &&
                        tContext->vertices[tContext->edgeLow[edge1]].y == tContext->vertices[tContext->edgeLow[edge2]].y) ? 3 : 5;
			}
			else if (pos == 1)
			{
                intersection->x = intersection->y = 0;

				return 2;
			}
		}
		else
		{
			gctINT32 pos = _PointLeftRight(tContext, tContext->edgeLow[edge2], edge1);
			if (pos == 0)
			{
				*intersection = tContext->vertices[tContext->edgeLow[edge2]];

				return (tContext->vertices[tContext->edgeLow[edge1]].x == tContext->vertices[tContext->edgeLow[edge2]].x &&
                        tContext->vertices[tContext->edgeLow[edge1]].y == tContext->vertices[tContext->edgeLow[edge2]].y) ? 3 : 6;
			}
			else if (pos == -1)
			{
                intersection->x = intersection->y = 0;

				return 2;
			}
		}

        x1 = tContext->vertices[tContext->edgeLow[edge1]].x;
		x2 = tContext->vertices[tContext->edgeLow[edge2]].x;
		y1 = tContext->vertices[tContext->edgeLow[edge1]].y;
		y2 = tContext->vertices[tContext->edgeLow[edge2]].y;
		dx1 = tContext->vertices[tContext->edgeHigh[edge1]].x - x1;
		dx2 = tContext->vertices[tContext->edgeHigh[edge2]].x - x2;
		dy1 = tContext->vertices[tContext->edgeHigh[edge1]].y - y1;
		dy2 = tContext->vertices[tContext->edgeHigh[edge2]].y - y2;

		xden = (_VGTessHighType)dx1*dy2 - (_VGTessHighType) dx2*dy1;
		xnum = (_VGTessHighType)dx1*dx2*(y1-y2) - (_VGTessHighType)dx2*dy1*x1 + (_VGTessHighType)dx1*dy2*x2;
		if (xden == 0)
		{
			_VGTesstype diff = TESS_ABS(dx1) + TESS_ABS(dy1) - TESS_ABS(dx2) - TESS_ABS(dy2);
			if (diff > SMALLEPSILON)
			{
                *intersection = tContext->vertices[tContext->edgeLow[edge2]];

				return 6;
			}
			else if (diff < -SMALLEPSILON)
			{
                *intersection = tContext->vertices[tContext->edgeLow[edge1]];

				return 5;
			}
			else
			{
                *intersection = tContext->vertices[tContext->edgeLow[edge1]];

				return 3;
			}
		}
		x = (_VGTesstype)(xnum / xden);

        z1 = TESS_ABS(dx1*dy2);
        z2 = TESS_ABS(dx2*dy1);
		if (z1 > z2)
		{
			y = (_VGTesstype)((_VGTessHighType)(x-x1)*dy1/dx1 + y1);
		}
		else if (z1 < z2)
		{
			y = (_VGTesstype)((_VGTessHighType)(x-x2)*dy2/dx2 + y2);
		}
		else
		{
			y = (TESS_ABS(dx1) > TESS_ABS(dx2)) ?
				(_VGTesstype)((_VGTessHighType)(x-x1)*dy1/dx1 + y1) :
			(_VGTesstype)((_VGTessHighType)(x-x2)*dy2/dx2 + y2);
		}

		highBottom = (edge1High) ? tContext->edgeLow[edge1] : tContext->edgeLow[edge2];
        if (tContext->vertices[highBottom].y - y > SMALLEPSILON)
		{
			intersection->x = intersection->y = 0;

			return 2;
		}
        else
        if (tContext->vertices[highBottom].y - y >= 0 &&
            tContext->vertices[highBottom].y - y <= SMALLEPSILON &&
            tContext->vertices[highBottom].x - x < -SMALLEPSILON)
		{
			intersection->x = intersection->y = 0;

			return 2;
		}
		lowTop = (_PointHigh(tContext, tContext->edgeHigh[edge1], tContext->edgeHigh[edge2])) ? tContext->edgeHigh[edge2] : tContext->edgeHigh[edge1];
        if (tContext->vertices[lowTop].y < y)
		{
            y = tContext->vertices[lowTop].y;
            x = tContext->vertices[lowTop].x;
		}
        if (tContext->vertices[lowTop].y == y && tContext->vertices[lowTop].x >= x)
		{
            x = tContext->vertices[lowTop].x + SMALLEPSILON;
		}

        intersection->x = x;
        intersection->y = y;
		if (TESS_ABS(x - x1) <= SMALLEPSILON && TESS_ABS(y - y1) <= SMALLEPSILON)
		{
            intersection->x = x1;
            intersection->y = y1;

			return 5;
		}
		else if (TESS_ABS(x - x2) <= SMALLEPSILON && TESS_ABS(y - y2) <= SMALLEPSILON)
		{
            intersection->x = x2;
            intersection->y = y2;

			return 6;
		}
		else
		{
            intersection->x = x;
            intersection->y = y;

			return 1;
		}
	}
}

/* Insert a point in the tree. The leaf for the region where the point is located becomes a node for
	* the point, and the 2 new regions become its left and right branch
	*/
/* Return value -9999 means out-of-memory. */
gctINT32 _AddPoint(
	_VGContext	*context,
    gctINT32 index
    )
{
	_VGTessellationContext	*tContext = &context->tessContext;
    gctINT32 i = 0;
    gctINT32 regionIndex = 0, lowPoint = 0;
	gctINT32 currPosition = tContext->pointsTreePosition[index];
	gctINT32 retVal = 0;
	gctINT32 pos;
#if USE_TA
	gcoOS os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x index=%d", context, index);

	while (tContext->tree[currPosition].type != 0)
	{	/* search through tree to find location */
		if (tContext->tree[currPosition].type == 1)
		{
			pos = _PointHigh3Way(tContext, index, tContext->tree[currPosition].objectIndex);
			if (pos == 0)
			{	/* share position */
				gcmFOOTER_ARG("return=%d", tContext->tree[currPosition].objectIndex + 1);

				return tContext->tree[currPosition].objectIndex + 1;
			}
			else
			{
				currPosition = (pos == 1) ? tContext->tree[currPosition].leftBranch : tContext->tree[currPosition].rightBranch;
			}
		}
		else
		{
			pos = _PointLeftRight2(tContext, index, tContext->tree[currPosition].objectIndex);
			if (pos == 0)
			{	/* point is on an edge */
				if (retVal == 0)
				{	/* point add to left if on edge */
					retVal = -1;
					currPosition = tContext->tree[currPosition].leftBranch;
				}
				else
				{
					currPosition = (index < retVal - 1) ? tContext->tree[currPosition].leftBranch : tContext->tree[currPosition].rightBranch;
				}
			}
			else
			{
				currPosition = (pos != 1) ? tContext->tree[currPosition].leftBranch : tContext->tree[currPosition].rightBranch;
			}
		}
	}
	tContext->tree[currPosition].leftBranch = tContext->treeTop + 1;
	tContext->tree[currPosition].rightBranch = tContext->treeTop + 2;
	if (tContext->treeTop >= tContext->treeLength - 2)
	{
#if USE_TA
		if (gcvSTATUS_OK !=
			TA_Resize(os, (void**)&tContext->tree, sizeof(_VGTreeNode) * (tContext->treeLength + tContext->totalPoints * 2), 1))
		{
			gcmFOOTER_ARG("return=%d", -9999);

			return -9999;
		}
#else
		_VGTreeNode *tempTree;
		OVG_MALLOC(context->os, tempTree, sizeof(_VGTreeNode) * (tContext->treeLength + tContext->totalPoints * 2));
        OVG_MEMSET(tempTree, 0, sizeof(_VGTreeNode) * (tContext->treeLength + tContext->totalPoints * 2));
		OVG_MEMCOPY(tempTree, tContext->tree, sizeof(_VGTreeNode) * tContext->treeLength);
        OVG_SAFE_FREE(context->os, tContext->tree);
		tContext->tree = tempTree;
#endif
        tContext->treeLength = tContext->treeLength + tContext->totalPoints * 2;
	}

    regionIndex = tContext->tree[currPosition].objectIndex;
	tContext->regionCounter++;
	tContext->tree[tContext->treeTop + 2].objectIndex = tContext->regionCounter;
	/* Define lower region */
	tContext->regions[tContext->regionCounter].upperVertex = index;
	tContext->regions[tContext->regionCounter].lowerVertex = tContext->regions[regionIndex].lowerVertex;
	tContext->regions[tContext->regionCounter].leftEdge = tContext->regions[regionIndex].leftEdge;
	tContext->regions[tContext->regionCounter].rightEdge = tContext->regions[regionIndex].rightEdge;
	tContext->regions[tContext->regionCounter].treeNode = tContext->treeTop + 2;
	tContext->regions[tContext->regionCounter].degen = tContext->regions[regionIndex].degen;
	tContext->tree[tContext->treeTop + 1].objectIndex = regionIndex;
	/* Shrink higher region */
	tContext->regions[regionIndex].lowerVertex = index;
	tContext->regions[regionIndex].treeNode = tContext->treeTop + 1;
	/* Change tree node to point  */
	tContext->tree[currPosition].type = 1;
	tContext->tree[currPosition].objectIndex = index;
	tContext->pointsTreePosition[index] = currPosition;
	tContext->pointsAdded[index] = gcvTRUE;
	tContext->treeTop += 2;
	tContext->regionBelow[index] = tContext->regionCounter;
	tContext->regionAbove[index][0] = regionIndex;
	lowPoint = tContext->regions[tContext->regionCounter].lowerVertex;
	if (lowPoint != -1)
	{
		for (i = 0; i < tContext->regionAboveLengths[lowPoint]; i++)
		{
			if (regionIndex == tContext->regionAbove[lowPoint][i])
			{
				tContext->regionAbove[lowPoint][i] = tContext->regionCounter;
				break;
			}
		}
	}

	gcmFOOTER_ARG("return=%d", retVal);

	return retVal;
}

gctINT32 _PointLeftRight(
	_VGTessellationContext	*tContext,
    gctINT32 pointIndex,
    gctINT32 edgeIndex
    )
{	/* -1 = left, 1 = right, 0 = on */
    _VGTesstype a = tContext->vertices[pointIndex].x;
	_VGTesstype b = tContext->vertices[pointIndex].y;
	_VGTesstype x1 = tContext->vertices[tContext->edgeHigh[edgeIndex]].x;
	_VGTesstype y1 = tContext->vertices[tContext->edgeHigh[edgeIndex]].y;
	_VGTesstype x2 = tContext->vertices[tContext->edgeLow[edgeIndex]].x;
	_VGTesstype y2 = tContext->vertices[tContext->edgeLow[edgeIndex]].y;

	if (y1 == y2)
	{
		return (b < y1 || (b == y1 && a < gcmMIN(x1,x2))) ? -1 : (b > y1 || (a > gcmMAX(x1,x2)) ? 1 : 0);
	}
	else
	{

		_VGTessHighType z = (_VGTessHighType)(x1-a)*(y1-y2) - (_VGTessHighType)(y1-b)*(x1-x2);

		return (z > SMALLEPSILON) ? -1 : ((z < -SMALLEPSILON) ? 1 : 0);
	}
}

gctINT32 _PointLeftRight2(
	_VGTessellationContext	*tContext,
    gctINT32 pointIndex,
    gctINT32 edgeIndex
    )
{	/* -1 = left, 1 = right, 0 = on */
    _VGTesstype a = tContext->vertices[pointIndex].x;
	_VGTesstype b = tContext->vertices[pointIndex].y;
	_VGTesstype x1 = tContext->vertices[tContext->edgeHigh[edgeIndex]].x;
	_VGTesstype y1 = tContext->vertices[tContext->edgeHigh[edgeIndex]].y;
	_VGTesstype x2 = tContext->vertices[tContext->edgeLow[edgeIndex]].x;
	_VGTesstype y2 = tContext->vertices[tContext->edgeLow[edgeIndex]].y;

	if (y1 == y2)
	{
		return (b < y1) ? -1 : ((b > y1) ? 1 : 0);
	}
	else
	{
		_VGTessHighType z = (_VGTessHighType)(x1-a)*(y1-y2) - (_VGTessHighType)(y1-b)*(x1-x2);

		return (z > SMALLEPSILON) ? -1 : ((z < -SMALLEPSILON) ? 1 : 0);
	}
}


gctINT32 _EdgeLeftRight(
	_VGTessellationContext	*tContext,
    gctINT32 edge1,
    gctINT32 edge2
    )
{	/* two Edges sharing same top point */
	return _EdgeLeftRight3(tContext, tContext->edgeHigh[edge1], tContext->edgeLow[edge1], tContext->edgeLow[edge2]);
}

gctINT32 _EdgeLeftRight3(
	_VGTessellationContext	*tContext,
    gctINT32 top,
    gctINT32 bottom1,
    gctINT32 bottom2
    )
{
	_VGTesstype a = tContext->vertices[top].x;
	_VGTesstype b = tContext->vertices[top].y;
	_VGTesstype x1 = tContext->vertices[bottom1].x;
	_VGTesstype y1 = tContext->vertices[bottom1].y;
	_VGTesstype x2 = tContext->vertices[bottom2].x;
	_VGTesstype y2 = tContext->vertices[bottom2].y;
	_VGTessHighType d = (a - x2) * (b - y1) - (a - x1) * (b - y2);
	return (gctINT32)TESS_SIGN(d);
}

/* This is to break several colinear edges by the same point. The region list includes all regions neghboring the edges
 * (1 more than number of edges) from left to right. The point (left point index) is inserted to the left most region,
 * and the first region in the list is the one above it.
 */
gceSTATUS _BreakEdgeBunch(
	_VGContext	*context,
    gctINT32    *regionList,
    gctINT32    regionListLength,
    gctINT32    leftPointIndex,
    gctINT32    edgeOffset
    )
{
    gctINT32 i;
	gctINT32 leftPointRegion = 0, theEdge = 0, newLeftPointIndex = 0, newEdgeIndex = 0, rightPointIndex = 0;
    _VGVector2	leftPoint, rightPoint;
	_VGTessellationContext	*tContext = &context->tessContext;

	gcmHEADER_ARG("context=0x%x regionList=0x%x regionListLength=%d leftPointIndex=%d edgeOffset=%d",
		context, regionList, regionListLength, leftPointIndex, edgeOffset);

	for (i = 0; i < regionListLength; i++)
	{
		if (tContext->regions[regionList[i]].lowerVertex == leftPointIndex)
		{
			leftPointRegion = i;
			break;
		}
	}
	for (i = leftPointRegion; i > 0; i--)
	{	/* left point index is somewhere in the middle */
		theEdge = tContext->regions[regionList[i]].leftEdge;
        leftPoint = tContext->vertices[leftPointIndex];
		newLeftPointIndex = tContext->totalPoints;
		newEdgeIndex = tContext->totalPoints + edgeOffset;
		if (tContext->totalPoints >= tContext->verticesLength)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				_ResizeArrays(context, tContext->totalPoints + 20))
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
		}
		tContext->totalPoints++;
        tContext->vertices[newLeftPointIndex] = leftPoint;
		tContext->pointsTreePosition[newLeftPointIndex] = tContext->regions[regionList[i-1]].treeNode;
		/* Out-of-Memory. */
		if (-9999 ==
			_AddPoint(context, newLeftPointIndex))
		{
			gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

			return gcvSTATUS_OUT_OF_MEMORY;
		}

		/* Out-of-Memory. */
		if (gcvSTATUS_OK !=
			_BreakEdge(context, theEdge, newEdgeIndex, regionList[i-1], regionList[i], newLeftPointIndex, leftPointIndex, 1))
		{
			gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

			return gcvSTATUS_OUT_OF_MEMORY;
		}
	}
	for (i = leftPointRegion; i < regionListLength - 1; i++)
	{
		theEdge = tContext->regions[regionList[i]].rightEdge;
        rightPoint = tContext->vertices[leftPointIndex];
		rightPointIndex = tContext->totalPoints;
		newEdgeIndex = tContext->totalPoints + edgeOffset;
		if (tContext->totalPoints >= tContext->verticesLength)
		{
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				_ResizeArrays(context, tContext->totalPoints + 20))
			{
				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
		}
		tContext->totalPoints++;
        tContext->vertices[rightPointIndex] = rightPoint;
		tContext->pointsTreePosition[rightPointIndex] = tContext->regions[regionList[i+1]].treeNode;
		/* Out-of-Memory. */
		if (-9999 ==
			_AddPoint(context, rightPointIndex))
		{
			gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

			return gcvSTATUS_OUT_OF_MEMORY;
		}

		/* Out-of-Memory. */
		if (gcvSTATUS_OK !=
			_BreakEdge(context, theEdge, newEdgeIndex, regionList[i], regionList[i+1], leftPointIndex, rightPointIndex, 0))
		{
			gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

			return gcvSTATUS_OUT_OF_MEMORY;
		}
	}

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
}

/* Starting from a region, set its adjecent regions, and their adjecent regions, and so on, to the given depth.
* When crossing an edge, the depth of the region on the other side of the edge will +1 or -1, according to the
* direction of the edge
*/
gctINT32    *_SetTrapezoidDepth(
	_VGContext	*context,
    gctINT32    index,
    gctINT32    depth,
    gctINT32    *length
	)
{
    gctINT32    i = 0, headCount = 0, currRegion = 0;
	gctINT32    *regionHead = gcvNULL;
    gctINT32    regionHeadLength;
	gctINT32    *tempHead = gcvNULL;

	gctINT32 lowVertex;
	gctINT32 tempDepth;
	_VGTessellationContext	*tContext = &context->tessContext;

	gcmHEADER_ARG("context=0x%x index=%d depth=%d length=0x%x",
		context, index, depth, length);

    *length = 0;

	if (tContext->trapezoidDepth[index] != -999)
	{
		gcmFOOTER_ARG("return=0x%x", regionHead);

		return regionHead;
	}

#if LOCAL_MEM_OPTIM
	_AllocateInt(tContext->IntPool, &regionHead, 200);
#else
	OVG_MALLOC(context->os, regionHead, sizeof(gctINT32) * 100 * 2);
	/* Out-of-Memory. */
	if (regionHead == gcvNULL)
	{
		gcmFOOTER_ARG("return=0x%x", gcvNULL);

		return gcvNULL;
	}
#endif
    regionHeadLength = 100;

	headCount = 0;
	currRegion = index;
	while (gcvTRUE)
	{
        lowVertex = 0;
		if (tContext->trapezoidDepth[currRegion] != -999)
		{
			break;
		}
		tContext->trapezoidDepth[currRegion] = depth;
		if (tContext->regions[currRegion].lowerVertex == -1)
		{
			break;
		}
		lowVertex = tContext->regions[currRegion].lowerVertex;
		if (tContext->regions[currRegion].leftEdge != -1 &&
            tContext->edgeLow[tContext->regions[currRegion].leftEdge] == lowVertex)
		{	/* left side ends */
			if (tContext->regions[currRegion].rightEdge != -1 &&
                tContext->edgeLow[tContext->regions[currRegion].rightEdge] == lowVertex)
			{	/* both sides end */
				if (depth % 2 != 0)
				{
					tContext->nextOdd[tContext->regions[currRegion].leftEdge] = tContext->regions[currRegion].rightEdge;
					tContext->edgeDirection[tContext->regions[currRegion].leftEdge] = 1;
					tContext->edgeDirection[tContext->regions[currRegion].rightEdge] = -1;
				}
				else if (depth != 0)
				{
					tContext->nextEven[tContext->regions[currRegion].leftEdge] = tContext->regions[currRegion].rightEdge;
				}
				break;
			}
			else if (tContext->regionBelow2[lowVertex] == 0)
			{	/* cusp up */
				if (depth % 2 != 0)
				{
					tContext->nextOdd[tContext->regions[currRegion].leftEdge] = tContext->regions[tContext->regionAbove[lowVertex][0]].rightEdge;
					tContext->edgeDirection[tContext->regions[currRegion].leftEdge] = 1;
				}
				else if (depth != 0)
				{
					tContext->nextEven[tContext->regions[currRegion].leftEdge] = tContext->regions[tContext->regionAbove[lowVertex][0]].rightEdge;
				}
				break;
			}
			else
			{
				if (depth % 2 != 0)
				{
					tContext->nextOdd[tContext->regions[currRegion].leftEdge] = tContext->regions[tContext->regionBelow2[lowVertex]].leftEdge;
					tContext->edgeDirection[tContext->regions[currRegion].leftEdge] = 1;
				}
				else if (depth != 0)
				{
					tContext->nextEven[tContext->regions[currRegion].leftEdge] = tContext->regions[tContext->regionBelow2[lowVertex]].leftEdge;
				}
				currRegion = tContext->regionBelow2[lowVertex];
			}
		}
		else
		{
			if ((headCount + tContext->regionBelow3Lengths[lowVertex] + 1) > regionHeadLength )
			{
#if LOCAL_MEM_OPTIM
				_AllocateInt(tContext->IntPool, &tempHead, (regionHeadLength + 100) * 2);
#else
				OVG_MALLOC(context->os, tempHead, sizeof(gctINT32) * (regionHeadLength + 100) * 2);
#endif
				OVG_MEMSET(tempHead, 0, sizeof(gctINT32*) * (regionHeadLength + 100) * 2);
                OVG_MEMCOPY(tempHead, regionHead, sizeof(gctINT32*) * regionHeadLength * 2);
#if LOCAL_MEM_OPTIM
				if (regionHead != gcvNULL)
				{
					_FreeInt(tContext->IntPool, regionHead);
					regionHead = gcvNULL;
				}
#else
                OVG_SAFE_FREE(context->os, regionHead);
#endif
				regionHead = tempHead;
                tempHead = gcvNULL;
				regionHeadLength += 100;
			}
			if (tContext->regions[currRegion].rightEdge != -1 &&
                tContext->edgeLow[tContext->regions[currRegion].rightEdge] == lowVertex)
			{	/* right side ends */
				if (tContext->regionBelow2[lowVertex] != 0)
				{
					if (depth % 2 != 0)
					{
						if (tContext->regions[tContext->regionBelow[lowVertex]].rightEdge != -1 ||
							!tContext->errorHandling)
						{
							tContext->nextOdd[tContext->regions[tContext->regionBelow[lowVertex]].rightEdge] = tContext->regions[currRegion].rightEdge;
						}
						tContext->edgeDirection[tContext->regions[currRegion].rightEdge] = -1;
					}
					else if (depth != 0)
					{
						tContext->nextEven[tContext->regions[tContext->regionBelow[lowVertex]].rightEdge] = tContext->regions[currRegion].rightEdge;
					}
				}
				else
				{
					if (depth % 2 != 0)
					{
						tContext->edgeDirection[tContext->regions[currRegion].rightEdge] = -1;
					}
				}
			}
			else
			{	/* region ends in middle */
				if (depth % 2 != 0)
				{
					tContext->nextOdd[tContext->regions[tContext->regionBelow[lowVertex]].rightEdge] = tContext->regions[tContext->regionBelow2[lowVertex]].leftEdge;
				}
				else if (depth != 0)
				{
					tContext->nextEven[tContext->regions[tContext->regionBelow[lowVertex]].rightEdge] = tContext->regions[tContext->regionBelow2[lowVertex]].leftEdge;
				}
				regionHead[headCount * 2 + 0] = tContext->regionBelow2[lowVertex];
				regionHead[headCount * 2 + 1] = depth;
				headCount++;
			}
			if (tContext->regionBelow3[lowVertex][0] != 0)
			{
				tempDepth = depth;
				for (i = 0; i < tContext->regionBelow3Lengths[lowVertex]; i++)
				{
					regionHead[headCount * 2 + 0] = tContext->regionBelow3[lowVertex][i];
					if (tContext->edgeUpDown[tContext->regions[tContext->regionBelow3[lowVertex][i]].leftEdge] == 1)
					{
						tempDepth++;
					}
					else
					{
						tempDepth--;
					}
					regionHead[headCount * 2 + 1] = tempDepth;
					headCount++;
					if (tempDepth % 2 != 0)
					{
						if (tContext->regions[tContext->regionBelow3[lowVertex][i]].rightEdge != -1 ||
							!tContext->errorHandling)
						{
							tContext->nextOdd[tContext->regions[tContext->regionBelow3[lowVertex][i]].rightEdge] = tContext->regions[tContext->regionBelow3[lowVertex][i]].leftEdge;
						}
					}
					else if (tempDepth != 0)
					{
						if (tContext->regions[tContext->regionBelow3[lowVertex][i]].rightEdge != -1 ||
							!tContext->errorHandling)
						{
							tContext->nextEven[tContext->regions[tContext->regionBelow3[lowVertex][i]].rightEdge] = tContext->regions[tContext->regionBelow3[lowVertex][i]].leftEdge;
						}
					}
				}
			}
			currRegion = tContext->regionBelow[lowVertex];
		}
	}


	if (headCount > 0)
	{
		tempHead = regionHead;
	}
	else
	{
#if LOCAL_MEM_OPTIM
		if (regionHead != gcvNULL)
		{
			_FreeInt(tContext->IntPool, regionHead);
			regionHead = gcvNULL;
		}
#else
		OVG_SAFE_FREE(context->os, regionHead);
#endif
	}

    *length = headCount;

	gcmFOOTER_ARG("return=0x%x", tempHead);

	return tempHead;
}

/* Forming monotone mountains inside regions with depth of given parity. The procedure:
* 1) In each region, if the 2 verteces defing the region are not on the same edge, join them with a diagonal;
* 2) At each vertex, order the diagonals (there are at most 3 of them) CCW;
* 3) Tracing moutains by these criteria: a. If arriving at a vertex by edge, go on to the first diagonal;
*    b. If arriving by a diagonal, go on to the next diagonal; c. If arriving by the last diagonal or by edge
*    to a vertex with no diagonal, continue to the following edge.
* 4) Make all edges (in one direction) and all diagonals (in both directions) are traced
*/
gceSTATUS	_MakeMountains(
	_VGContext	*context,
    gctINT32    parity,
    gctINT32    *numMountains,
    gctINT32    **lengthArray
    )
{
    gctINT32    i, j = 0, diagonalCounter = 0, startPoint = 0, count = 0, nextPoint = 0, diagonalFrom = 0, goPath = 0;
    gctINT32    *nextEdge = gcvNULL, *pointToTrace = gcvNULL;
#if LOCAL_MEM_OPTIM
	gctINT32    **mountains = gcvNULL;
#endif
	gctINT32    *mountain = gcvNULL, currMountain = 0;
	gctINT32    *diagonal = gcvNULL;
    gctINT32    diagonalLength;
	gctBOOL		willBreak;
	gctINT32	allocated;

	gctINT32	lowVertex;
	gctINT32	highVertex;
	gctINT32	lowPreEdge, highPreEdge, lowOrder, highOrder;
	gctINT32	dCount;
	gctINT32	toTrace;
	_VGTessellationContext	*tContext = &context->tessContext;
#if USE_TA
	gcoOS		os = context->os;
#endif

	gcmHEADER_ARG("context=0x%x parity=%d numMountains=0x%x lengthArray=0x%x",
		context, parity, numMountains, lengthArray);

#if LOCAL_MEM_OPTIM
	_AllocateInt(tContext->IntPool, &diagonal, tContext->totalPoints * 3);
#else
	OVG_MALLOC(context->os, diagonal, sizeof(gctINT32) * tContext->totalPoints * 3);
	/* Out-of-Memory. */
	if (diagonal == gcvNULL)
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}

#endif
    diagonalLength = tContext->totalPoints;
	OVG_MEMSET(diagonal, 0xFF, sizeof(gctINT32) * tContext->totalPoints * 3);

	/* Add diagonals */
	for (i = 0; i < tContext->regionsLength; i++)
	{
		if ((tContext->trapezoidDepth[i] & 1) == parity &&
            tContext->trapezoidDepth[i] != 0)
		{
			if ((tContext->regions[i].leftEdge != -1 &&
                 tContext->edgeLow[tContext->regions[i].leftEdge] == tContext->regions[i].lowerVertex &&
				 tContext->edgeHigh[tContext->regions[i].leftEdge] == tContext->regions[i].upperVertex) ||
				(tContext->regions[i].rightEdge != -1 &&
                 tContext->edgeLow[tContext->regions[i].rightEdge] == tContext->regions[i].lowerVertex &&
				 tContext->edgeHigh[tContext->regions[i].rightEdge] == tContext->regions[i].upperVertex))
			{	/* do not add */
				continue;
			}
			else
			{
				/* Add one diagonal
				 diagonal order at each point is important when tracing mountains */
				lowVertex = tContext->regions[i].lowerVertex;
				highVertex = tContext->regions[i].upperVertex;

				diagonalCounter++;

				if (tContext->regions[i].leftEdge != -1 &&
                    tContext->edgeLow[tContext->regions[i].leftEdge] == lowVertex)
				{
					lowPreEdge = tContext->regions[i].leftEdge;
					lowOrder = 0;
				}else
                if (tContext->regions[i].rightEdge != -1 &&
                    tContext->edgeLow[tContext->regions[i].rightEdge] == lowVertex &&
					tContext->regionBelow2[lowVertex] == 0)
				{
					lowPreEdge = tContext->regions[tContext->regionAbove[lowVertex][tContext->regionAboveLengths[lowVertex] - 1]].leftEdge;
					lowOrder = 2;
				}
				else
				{
					lowPreEdge = tContext->regions[tContext->regionBelow[lowVertex]].rightEdge;
					lowOrder = 1;
				}

				if (tContext->regions[i].rightEdge != -1 &&
                    tContext->edgeHigh[tContext->regions[i].rightEdge] == highVertex)
				{
					highPreEdge = tContext->regions[i].rightEdge;
					highOrder = 0;
				}else
                if (tContext->regions[i].leftEdge != -1 &&
                    tContext->edgeHigh[tContext->regions[i].leftEdge] == highVertex &&
					tContext->regionAboveLengths[highVertex] == 1)
				{
					highPreEdge = tContext->regions[tContext->regionBelow[highVertex]].rightEdge;
					highOrder = 2;
				}
				else
				{
					highPreEdge = tContext->regions[tContext->regionAbove[highVertex][tContext->regionAboveLengths[highVertex] - 1]].leftEdge;
					highOrder = 1;
				}

				if (lowPreEdge != -1 || !tContext->errorHandling)
				{
					diagonal[lowOrder * tContext->totalPoints + lowPreEdge] = highPreEdge;
				}
				if (highPreEdge != -1 || !tContext->errorHandling)
				{
					diagonal[highOrder * tContext->totalPoints + highPreEdge] = lowPreEdge;
				}
			}
		}
	}

	/* Set correct diagonal order at each vertex, and the (binary) tracing flags */
	nextEdge = (parity == 1) ? tContext->nextOdd : tContext->nextEven;
#if LOCAL_MEM_OPTIM
	_AllocateInt(tContext->IntPool, &pointToTrace, tContext->totalPoints);
#else
	OVG_MALLOC(context->os, pointToTrace, sizeof(gctINT32) * tContext->totalPoints);
	/* Out-of-Memory. */
	if (pointToTrace == gcvNULL)
	{
		goto on_out_of_memory;
	}

#endif
    OVG_MEMSET(pointToTrace, 0, sizeof(gctINT32) * tContext->totalPoints);

	for (i = 0; i < tContext->totalPoints; i++)
	{
		if (nextEdge[i] != -1)
		{
			dCount = 0;
			toTrace = 2;
			if (diagonal[diagonalLength * 0 + i] != -1)
			{
				dCount++;
				toTrace *= 2;
			}
			if (diagonal[diagonalLength * 1 + i] != -1)
			{
				diagonal[diagonalLength * (dCount++) + i] = diagonal[diagonalLength * 1 + i];
				toTrace *= 2;
			}
			if (diagonal[diagonalLength * 2 + i] != -1)
			{
				diagonal[diagonalLength * (dCount++) + i] = diagonal[diagonalLength * 2 + i];
				toTrace *= 2;
			}
			for (j = dCount; j < 3; j++)
			{
				diagonal[diagonalLength * j + i] = -1;
			}
			pointToTrace[i] = toTrace - 1;
		}
	}

#if LOCAL_MEM_OPTIM
	_AllocateIntp(tContext->IntpPool, &mountains, tContext->totalPoints);
#else
	/* Out-of-Memory. */
	if (gcvSTATUS_OK !=
		TA_Malloc(os, (void**)&tContext->mountains, sizeof(gctINT32*) * tContext->totalPoints, 2))
	{
		goto on_out_of_memory;
	}
#endif
    if (lengthArray != gcvNULL)
    {
#if LOCAL_MEM_OPTIM
		_AllocateInt(tContext->IntPool, lengthArray, tContext->totalPoints);
#else
		OVG_MALLOC(context->os, *lengthArray, sizeof(gctINT32) * tContext->totalPoints);
		/* Out-of-Memory. */
		if (*lengthArray == gcvNULL)
		{
			goto on_out_of_memory;
		}
#endif
    }
	/* tracing mountains */
	startPoint = 0;
	currMountain = 0;
	while (startPoint < tContext->totalPoints)
	{
		if (pointToTrace[startPoint] == 0)
		{
			startPoint++;
		}
		else
		{
			allocated = 20;
#if LOCAL_MEM_OPTIM
			_AllocateInt(tContext->IntPool, &mountain, allocated);
#else
			/* Out-of-Memory. */
			if (gcvSTATUS_OK !=
				TA_Malloc(os, (void**)&tContext->mountains[currMountain], sizeof(gctINT32) * allocated, 1))
			{
				goto on_out_of_memory;
			}
			mountain = tContext->mountains[currMountain];
#endif
			count = 0;
			nextPoint = startPoint;
			diagonalFrom = -1;
			do
			{
				willBreak = gcvFALSE;
				if (tContext->errorHandling && (nextPoint == -1))
                    break;
				mountain[count++] = (tContext->edgeDirection[nextPoint] == 1) ? tContext->edgeLow[nextPoint] : tContext->edgeHigh[nextPoint];

                if (tContext->errorHandling && (count == tContext->totalPoints))
					willBreak = gcvTRUE;
				if (tContext->errorHandling && (count > 1) && (mountain[count-1] == mountain[count-2]))
				{
					count--;
                    break;
				}
				if (count > 1 && _PointHigh3Way(tContext, mountain[count-1], mountain[count-2]) == 0)
				{
					count--;
				}

				/* Find out where to go next */
				goPath = 0;
				if (pointToTrace[nextPoint] > 1)
				{
					if (diagonalFrom == -1)
					{
						if ((pointToTrace[nextPoint] & 2) == 2)
						{
							goPath = 1;
						}
						else if ((pointToTrace[nextPoint] & 4) == 4)
						{
							goPath = 2;
						}
						else
						{
							goPath = 3;
						}
					}
					else
					{
						/* j==0 */
						if (diagonal[nextPoint] == diagonalFrom)
						{
							if (diagonal[diagonalLength + nextPoint] == -1)
								goPath = 0;
							else
								goPath = 2;
						}
						else	/* j==1 */
						if (diagonal[diagonalLength + nextPoint] == diagonalFrom)
						{
							if (diagonal[diagonalLength * 2 + nextPoint] == -1)
								goPath = 0;
							else
								goPath = 3;
						}
						else	/* j==2 */
						if (diagonal[diagonalLength * 2 + nextPoint] == diagonalFrom)
							goPath = 0;
					}
				}
				switch (goPath)
				{
					case 0:
                        {
						pointToTrace[nextPoint] &= 14;	/* remove 1 */
						diagonalFrom = -1;
						nextPoint = nextEdge[nextPoint];
                        }
						break;
					case 1:
                        {
						pointToTrace[nextPoint] &= 13;	/* remove 2 */
						diagonalFrom = nextPoint;
						nextPoint = diagonal[nextPoint];
                        }
						break;
					case 2:
                        {
						pointToTrace[nextPoint] &= 11;	/* remove 4 */
						diagonalFrom = nextPoint;
						nextPoint = diagonal[diagonalLength + nextPoint];
                        }
						break;
					case 3:
                        {
						pointToTrace[nextPoint] &= 7;	/* remove 8 */
						diagonalFrom = nextPoint;
						nextPoint = diagonal[diagonalLength * 2 + nextPoint];
                        }
						break;
				}
				if (willBreak)
					break;

				if (count > allocated - 2)
				{
					allocated += 50;
					/* Out-of-Memory. */
					if (gcvSTATUS_OK !=
						TA_Resize(os, (void**)&tContext->mountains[currMountain], sizeof(gctINT32) * allocated, 1))
					{
						goto on_out_of_memory;
					}

					mountain = tContext->mountains[currMountain];
				}
			} while (nextPoint != startPoint);
			if (_PointHigh3Way(tContext, mountain[0], mountain[count-1]) == 0)
			{	/* if they are the same intersection point */
				count--;
			}
			if (count > 2)
			{	/* some intersection mountains have only 2 points*/
                if (lengthArray != gcvNULL)
                {
                    (*lengthArray)[currMountain] = count;
                }
				currMountain++;
			}
			else
			{
#if LOCAL_MEM_OPTIM
				if (mountain != gcvNULL)
				{
					_FreeInt(tContext->IntPool, mountain);
					mountain = gcvNULL;
				}
#else
#endif
			}
		}
	}
    if (numMountains != gcvNULL)
    {
        *numMountains = currMountain;
    }

#if LOCAL_MEM_OPTIM
	if (pointToTrace != gcvNULL)
	{
		_FreeInt(tContext->IntPool, pointToTrace);
		pointToTrace = gcvNULL;
	}
	if (diagonal != gcvNULL)
	{
		_FreeInt(tContext->IntPool, diagonal);
		diagonal = gcvNULL;
	}
#else
    OVG_SAFE_FREE(context->os, pointToTrace);
    OVG_SAFE_FREE(context->os, diagonal);
#endif
	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;

on_out_of_memory:
    OVG_SAFE_FREE(context->os, pointToTrace);
    OVG_SAFE_FREE(context->os, diagonal);

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

	return gcvSTATUS_OUT_OF_MEMORY;
}

/* Clip triangles off a monotone mountain. Procedure:
* 1) Rearrange verteces to top-down
* 2) Move down to find the first convex vertex, and clip it off
* 3) Move back one step and check the convexity. If Yes, clip it off.
* 4) If no, move down to find the first convex vertex, and clip it off
* 5) Continue until all verteces clipped
*/
gceSTATUS _MoveMountain(
	_VGContext	*context,
    gctINT32 *mountain,
    gctINT32 mountainLength
    )
{
    gctINT32 i;
    gctINT32 direction = 0, i0, i1;
    gctINT32 *tempMountain = gcvNULL, *cut = gcvNULL;
    gctINT32 up = 0, down = 0, middle = 0;

    _VGTesstype x1, y1;
	_VGTesstype x2, y2;
	_VGTesstype x3, y3;
	_VGTessHighType outer;
	_VGTessellationContext	*tContext = &context->tessContext;
	_VGint16	*tempIndx = tContext->triangles + tContext->triangleCounter * 3;

	gcmHEADER_ARG("context=0x%x mountain=0x%x mountainLength=%d",
		context, mountain, mountainLength);

	if (mountainLength == 3)
	{
		tempIndx[0] = (_VGuint16)mountain[0];
		tempIndx[1] = (_VGuint16)mountain[1];
		tempIndx[2] = (_VGuint16)mountain[2];
		tContext->triangleCounter++;

		gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

		return gcvSTATUS_OK;
	}

	/* 0 is top, down to bottom */
	i0 = 0;
    i1 = 1;
#if LOCAL_MEM_OPTIM
	_AllocateInt(tContext->IntPool, &tempMountain, mountainLength);
#else
	OVG_MALLOC(context->os, tempMountain, sizeof(gctINT32) * mountainLength);

	/* Out-of-Memory. */
	if (tempMountain == gcvNULL)
	{
		goto on_mem_error;
	}
#endif

	while (_PointHigh(tContext, mountain[i1],mountain[i0]))
	{
		i0 = i1;
		if (i0 == mountainLength - 1)
		{
			i1 = 0;
			break;
		}
		i1++;
	}
	if (i0 == 0)
	{
		while (_PointHigh(tContext, mountain[i0],mountain[i1]))
		{
			i0 = i1;
			if (i0 == mountainLength - 1)
			{
				i1 = 0;
				break;
			}
			i1++;
		}
		/* i0 is lowest */
		if (_PointHigh(tContext, mountain[i1], mountain[i0-1]))
		{	/* i1 highest */
			for (i = 0; i < mountainLength; i++)
			{
				tempMountain[i] = mountain[(i1+i) % mountainLength];
			}
			direction = 1;
		}
		else
		{	/* i0-1 highest */
			for (i = 0; i < mountainLength; i++)
			{
				tempMountain[i] = mountain[(mountainLength + i0 - 1 - i) % mountainLength];
			}
			direction = -1;
		}
	}
	else
	{	/* i0 is highest */
		if (_PointHigh(tContext, mountain[i1], mountain[i0 - 1]))
		{	/* i0-1 lowest */
			for (i = 0; i < mountainLength; i++)
			{
				tempMountain[i] = mountain[(i0 + i) % mountainLength];
			}
			direction = 1;
		}
		else
		{	/* i1 lowest */
			for (i = 0; i < mountainLength; i++)
			{
				tempMountain[i] = mountain[(mountainLength + i0 - i) % mountainLength];
			}
			direction = -1;
		}
	}

	mountain = tempMountain;

#if LOCAL_MEM_OPTIM
	_AllocateInt(tContext->IntPool, &cut, mountainLength);
#else
	OVG_MALLOC(context->os, cut, sizeof(gctINT32) * mountainLength);

	/* Out-of-Memory. */
	if (cut == gcvNULL)
	{
		goto on_mem_error;
	}
#endif
    OVG_MEMSET(cut, 0, sizeof(gctINT32) * mountainLength);
	up=0, down=2, middle=1;
	tempIndx++;
	while (up > 0 || down < mountainLength - 1)
	{
        x1 = tContext->vertices[mountain[up]].x;
		y1 = tContext->vertices[mountain[up]].y;
		x2 = tContext->vertices[mountain[middle]].x;
		y2 = tContext->vertices[mountain[middle]].y;
		x3 = tContext->vertices[mountain[down]].x;
		y3 = tContext->vertices[mountain[down]].y;
		outer = ((x2 - x1) * (y3 - y2) - (x3 - x2) * (y2 - y1));
		if ((direction == -1 && outer<=0) ||
            (direction == 1 && outer>=0)  ||
            (tContext->errorHandling && (down == mountainLength - 1)))
		{	/* a convex point */
			gcmASSERT(direction != 0);
			*tempIndx = (_VGuint16)mountain[middle];
			*(tempIndx - direction) = (_VGuint16)mountain[up];
			*(tempIndx + direction) = (_VGuint16)mountain[down];
			tempIndx += 3;
			tContext->triangleCounter++;
			cut[middle] = 1;
			if (up > 0 || (tContext->errorHandling && (down == mountainLength - 1)))
			{
				middle = up;
				up--;
				while (cut[up]==1)
				{
					up--;
				}
			}
			else
			{
				middle = down;
				down++;
			}
		}
		else
		{
			up = middle;
			middle = down;
			down++;
		}
	}
	gcmASSERT(direction != 0);
	*tempIndx = (_VGuint16)mountain[middle];
	*(tempIndx - direction) = (_VGuint16)mountain[up];
	*(tempIndx + direction) = (_VGuint16)mountain[down];
	tContext->triangleCounter++;

#if LOCAL_MEM_OPTIM
	if (cut != gcvNULL)
	{
		_FreeInt(tContext->IntPool, cut);
	}
	if (tempMountain != gcvNULL)
	{
		_FreeInt(tContext->IntPool, tempMountain);
	}
#else
    OVG_SAFE_FREE(context->os, cut);
    OVG_SAFE_FREE(context->os, tempMountain);

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
#endif

on_mem_error:
    OVG_SAFE_FREE(context->os, cut);
    OVG_SAFE_FREE(context->os, tempMountain);

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

	return gcvSTATUS_OUT_OF_MEMORY;
}

gctINT32	_GetRadAngle(
	_VGTesstype unitx,
	_VGTesstype unity
	)
{
    gctINT32 l, h, m;

	gcmHEADER_ARG("unitx=%f unity=%f", unitx, unity);

    l = 0;
    h = 180;
    m = 90;
    while (m != l && m != h)
    {
        if (unitx < CIRCLE_COS[m])
            l = m;
        else
        if (unitx > CIRCLE_COS[m])
            h = m;
        else
            break;

        m = (l + h) >> 1;
    }
    if (unity < 0)
    {
        m = 360 - m;
    }

	gcmFOOTER_ARG("return=%d", m);

	return m;
}

gctINT32	_GetCirclePointCount(
	gctINT32	angle0,		/* starting angle, By degree */
	gctINT32	angle1,		/* ending angle, By degree */
	gctINT32	step
	)
{
	gcmASSERT(angle0 >= 0);
	gcmASSERT(angle1 >= angle0);

	if (angle1 < angle0)
	{
		return 0;
	}

	/* Determin precesion steps. */
	return (angle1 - angle0) / step + 1;
}

void	_FlattenCircle(
	_VGVector2 *center,
	gctFLOAT	radius,
	gctINT32	sp,
	gctINT32	ep,
	gctINT32	step,
	_VGVector2	*points
	)
{
	int i;

	gcmHEADER_ARG("center=0x%x radius=%f sp=%d ep=%d step=%d points=0x%x",
		center, radius, sp, ep, step, points);

	gcmASSERT(points != gcvNULL);
	if (points == gcvNULL)
	{
		gcmFOOTER_NO();
		return;
	}

	if (ep < 360)
	{
		for (i = sp; i <= ep; i += step)
		{
			points->x = center->x + radius * CIRCLE_COS[i];
			points->y = center->y + radius * CIRCLE_SIN[i];
			points++;
		}
	}
	else
	{
		for (i = sp; i < 360; i += step)
		{
			points->x = center->x + radius * CIRCLE_COS[i];
			points->y = center->y + radius * CIRCLE_SIN[i];
			points++;
		}
		i -= 360;
		ep -= 360;
		for (; i <= ep; i += step)
		{
			points->x = center->x + radius * CIRCLE_COS[i];
			points->y = center->y + radius * CIRCLE_SIN[i];
			points++;
		}
	}

	gcmFOOTER_NO();
}

void	_ExpandPipe(
					_VGContext	*context,
					_StreamPipe *pipe,
					int			 vertexMin,
					int			 indexMin
					)
{
	void *temp;
	int sizeVert, sizeInd;

	gcmHEADER_ARG("context=0x%x pipe=0x%x vertexMin=%d indexMin=%d",
		context, pipe, vertexMin, indexMin);

	sizeVert = (int)(gcmMAX(vertexMin, pipe->numStreamPts) * 1.2f);
	sizeInd = (int)(gcmMAX(indexMin, pipe->numIndices) * 1.2f);

	if (vertexMin >= pipe->numStreamPts)
	{
		OVG_MALLOC(context->os, temp, sizeof(_VGVector2) * sizeVert);
		OVG_MEMCOPY(temp, pipe->stream, sizeof(_VGVector2) * pipe->currStreamPts);
		OVG_SAFE_FREE(context->os, pipe->stream);
		pipe->stream = (_VGVector2*)temp;
		pipe->numStreamPts = sizeVert;
	}

	if (indexMin >= pipe->numIndices)
	{
		OVG_MALLOC(context->os, temp, sizeof(_VGuint16) * sizeInd);
		OVG_MEMCOPY(temp, pipe->indices, sizeof(_VGuint16) * pipe->currIndex);
		OVG_SAFE_FREE(context->os, pipe->indices);
		pipe->indices = (_VGuint16*)temp;
		pipe->numIndices = sizeInd;
	}

	gcmFOOTER_NO();
}

#if USE_TA
/* Triangulation Specific Memory Operations. */
/* 1-dim array operations. Generic data structure. */
/* Reserve the first 4 bytes to save the size of the requested memory, the followed by the actually memory for the client to use. */
void	*TA_Init(gcoOS os, int size, int levels)
{
	int *result;
	gceSTATUS status;

	gcmHEADER_ARG("os=0x%x, size=%d, levels=%d", os, size, levels);

	status = gcoOS_Allocate(os, size + sizeof(int), (gctPOINTER*)&result);

	/* Handle Out-of_Memory. */
	if (status != gcvSTATUS_OK)
	{
		gcmFOOTER_ARG("return=0x%x", gcvNULL);

		return gcvNULL;
	}

	*result = size;
	result++;
	if (levels == 2)	/* a 2-level pointer, only for Int** */
	{
		int i;
		int	**temp = (int**)result;
		size /= sizeof(int);
		for (i = 0; i < size; i++)
		{
			temp[i] = (int*)TA_Init(os, sizeof(int) * 10, 1);

			/* Out-of-Memory. */
			if (temp[i] == gcvNULL)
			{
				break;
			}
		}

		/* Out-of-Memory. */
		if (i < size)
		{
			/* Free each element. */
			while (i >= 0)
			{
				if (temp[i] != gcvNULL)
				{
					TA_Destroy(os, (void**)&temp[i], 1);
				}
				i--;
			}

			/* Free the array. */
			result--;
			gcmOS_SAFE_FREE(os, result);
		}
	}
	else
	{
		gcoOS_MemFill(result, 0, size);
	}

	gcmFOOTER_ARG("return=0x%x", result);

	return (void*)result;
}

/* The pointer must be a TA conformant type. */
gceSTATUS	TA_Malloc(gcoOS os, void **pointer, int size, int levels)
{
	int length = GET_TASIZE(*pointer);

	gcmHEADER_ARG("os=0x%x pointer=0x%x size=%d levels=%d",
		os, pointer, size, levels);

	if (length > size)		/* Reuse the current array buffer. */
	{
		if (levels == 1)
			gcoOS_MemFill(*pointer, 0, length);
	}
	else					/* Create a newer and bigger array buffer for use. And the exting data is destroyed.*/
	{
		TA_Destroy(os, pointer, levels);
		*pointer = TA_Init(os, size, levels);
	}

	/* Out-of-Memory. */
	if (*pointer == gcvNULL)
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

		return gcvSTATUS_OUT_OF_MEMORY;
	}
	else
	{
		gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

		return gcvSTATUS_OK;
	}
}

/* The same as TA_Malloc except that the existing data is reserved. */
gceSTATUS	TA_Resize(gcoOS os, void **pointer, int newsize, int levels)
{
	int length = GET_TASIZE(*pointer);

	gcmHEADER_ARG("os=0x%x pointer=0x%x newsize=%d levels=%d",
		os, pointer, newsize, levels);

	if (length < newsize)
	{
		int i;
		int *result;
		gceSTATUS status;
		newsize += sizeof(int) * 10;
		status = gcoOS_Allocate(os, newsize + sizeof(int), (gctPOINTER*)&result);

		/* Out-of-Memory. */
		if (status != gcvSTATUS_OK)
		{
			gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

			return gcvSTATUS_OUT_OF_MEMORY;
		}

		*result = newsize;
		result++;
		gcoOS_MemFill(result, 0, newsize);
		gcoOS_MemCopy(result, *pointer, length);	 /* Copy the old data */

		if (levels == 2)	/* Handle elements for 2-level pointer */
		{
			/* Malloc for the rest elements.*/
			int count = length / sizeof(int*);
			int **tempp = (int**)(result + count);
			count = newsize / sizeof(int*) - count;
			for (i = 0; i < count; i++)
			{
				tempp[i] = TA_Init(os, sizeof(int) * 10, 1);

				/* Out-of-Memory. */
				if (tempp[i] == gcvNULL)
				{
					break;
				}
			}

			/* Out-of-Memory. */
			if (i < count)
			{
				while (i >= 0)
				{
					if (tempp[i] != gcvNULL)
					{
						TA_Destroy(os, (void**)&tempp[i], 1);
					}
					i--;
				}

				result--;
				gcmOS_SAFE_FREE(os, result);

				gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);

				return gcvSTATUS_OUT_OF_MEMORY;
			}
		}
		TA_Destroy(os, pointer, 1);
		*pointer = result;
	}

	gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);

	return gcvSTATUS_OK;
}

void	TA_Free(gcoOS os, void **pointer)
{
	/* Do nothing. */
}

void	TA_Destroy(gcoOS os, void **pointer, int levels)
{
	/* Do free the actually memory. */
	int i;
	char *temp;

	gcmHEADER_ARG("os=0x%x pointer=0x%x levels=%d", os, pointer, levels);

	if (levels == 2)
	{
		int size = GET_TASIZE(*pointer);
		int **tempp = (int**)(*pointer);
		size /= sizeof(int*);
		for (i = 0; i < size; i++)
		{
			TA_Destroy(os, (void**)&tempp[i], 1);
		}
	}
	temp = (char*)(*pointer) - sizeof(int);
	gcoOS_Free(os, (gctPOINTER)temp);
	temp = gcvNULL;
	*pointer = gcvNULL;

	gcmFOOTER_NO();
}
#endif

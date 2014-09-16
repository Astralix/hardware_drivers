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


#ifndef __gc_vgsh_tessellator_h_
#define __gc_vgsh_tessellator_h_

/* Vincent: Fill Tessellation. ***********************************************/
    #define  TESSTYPE(x)            (_VGTesstype)(x)
    #define  TESS2TEMPLATE(x)       (_VGTemplate)((x))
    #define  TESS_INT(x)            ((gctINT32)x)
    #define  _VGTesstype            gctFLOAT
    #define  _VGTessHighType        gctFLOAT
    #define  TESS_CONST_ONE_FOURTH  0.25f
    #define  TESS_CONST_HALF        0.5f
    #define  TESS_CONST_ZERO        0.0f
    #define  TESS_CONST_ONE         1.0f
    #define  TESS_CONST_4           4.0f
    #define  TESS_CONST_2           2.0f
    #define  TESS_CONST_3           3.0f
    #define  TESS_CONST_1_3RD       0.3333333333333f
    #define  TESS_CONST_1_3465      1.3465f
    #define  TESS_CONST_0_3465      0.3465f
    #define  TESS_CONST_0_43        0.43f
    #define  TESS_CONST_0_3         0.3f
    #define  TESS_CONST_N0_25733    (-0.25733f)
    #define  TESS_CONST_0_80676     0.80676f
    #define  TESS_CONST_0_45056     0.45056f
    #define  TESS_CONST_N1_80076    (-1.80076f)
    #define  TESS_CONST_1_74668     1.74668f
    #define  TESS_CONST_0_3075      0.3075f
    #define  TESS_CONST_1_08        1.08f
    #define  TESS_CONST_0_001       0.001f
    #define  TESS_CONST_0_1         0.1f
    #define  TESS_PI_DOUBLE         (gcdPI * 2.0f)
    #define  TESS_PI_OCT            (gcdPI / 8.0f)
    #define  TESS_PI_RCP            (1.0f / gcdPI)
    #define  TESS_PI_DIV_180        PI_DIV_180_F

    #define  TESS_MUL(x1, x2)       (_VGTesstype)gcoMATH_Multiply(x1, x2)
    #define  TESS_DIV(x1, x2)       (_VGTesstype)gcoMATH_Divide(x1, x2)
    #define  TESS_SIN(a)            (_VGTesstype)gcoMATH_Sine(a)
    #define  TESS_COS(a)            (_VGTesstype)gcoMATH_Cosine(a)
    #define  TESS_SQRT(a)           (_VGTesstype)gcoMATH_SquareRoot(a)
    #define  TESS_ACOS(a)           (_VGTesstype)gcoMATH_ArcCosine(a)
    #define  TESS_FLOOR(a)          (_VGTesstype)gcoMATH_Floor(a)
    #define  TESS_ABS(a)            (_VGTesstype)gcoMATH_Absolute(a)
    #define  TESS_CEIL(a)           (_VGTesstype)gcoMATH_Ceiling(a)
    #define  TESS_DIAG(a, b)        gcoMATH_SquareRoot((a)*(a) + (b)*(b))
    #define  TESS_SIGN(a)           ((a > 0.0f) ? 1.0f : (a == 0.0f ? 0.0f : -1.0f))
    #define  UN_INIT_TAN_VALUE      ((float)(0xdeadbeef))

    /* Flattened points smooth type. */
    #define     POINT_FLAT          0
    #define     POINT_SMOOTH        1
    #define     POINT_SMOOTH_IN     2
    #define     POINT_SMOOTH_OUT    4

    #define     BEZIER_MOST_STEPS 12

    #define     VERTEX_TYPE gcvVERTEX_FLOAT
    #define     EPSILON         0.22f
    #define     EPSILON_SQUARE  0.0484f

    #define ZEROERROR   0.00001f
    #define SMALLEPSILON 0.000001f
    #define ANGLE_EPSILON2 0.0048659656292148f

typedef struct _TessPoint
{
    _VGVector2      coord;
    _VGVector2      inTan;
    _VGVector2      outTan;
    _VGTesstype     distance2Next;
    _VGuint8        flags;      /* Bit0: 0, flat segment; 1, smooth segment */
}_VGTessPoint;

typedef struct _VGFlattenBuffer
{
    gctINT32        numPoints;
    _VGTessPoint    *points;
    gctINT32        *numPointsInSubPath;
    gctBOOL         *subPathClosed;
    gctINT32        numSubPaths;
}_VGFlattenBuffer;

typedef struct _Bezier
{
    _VGVector2      cp[4];      /* Control Points. */
}_VGBezier;

typedef struct _Ellipse
{
    _VGVector2      center;     /* Center of the elipse. */
    _VGTesstype     hAxis;      /* Horizontal axis length. */
    _VGTesstype     vAxis;      /* Vertical axis length. */
    _VGTesstype     angle;      /* The angle of the eliptic arc. */
}_VGEllipse;

typedef struct _StreamPipe
{
    _VGVector2      *stream;
    gctINT32        currStreamPts;
    gctINT32        numStreamPts;
    _VGuint16       *indices;
    gctINT32        currIndex;
    gctINT32        numIndices;
}_StreamPipe;

/* Path Segment Shape Object. Mainly used in query path points and tangents. */
typedef struct _VGShapeMove
{
    _VGVector2  point;
}_VGShapeMove;
typedef struct _VGShapeClose
{
    _VGVector2  points[2];
}_VGShapeClose;
typedef struct _VGShapeLine
{
    _VGVector2  points[2];
}_VGShapeLine;
typedef struct _Bezier      _VGShapeBezier;
typedef struct _VGShapeArc
{
    _VGVector2  points[2];
    _VGEllipse  ellipse;
}_VGShapeArc;
typedef struct _VGShapeObject
{
    _VGuint8        type;
    _VGVector2      *fromP;
    _VGVector2      *toP;
    union
    {
        _VGShapeMove    move;
        _VGShapeClose   close;
        _VGShapeLine    line;
        _VGShapeBezier  bezier;
        _VGShapeArc     arc;
    } object;
}_VGShapeObject;

typedef struct _VGPathSegInfo
{
    gctINT32        startPoint;         /* Which point in the flattened result this segment starts from. */
    gctINT32        numPoints;          /* How many points this seg has in the flattened result. */
    _VGTesstype     length;
    _VGVector2      inTan, outTan;
    _VGShapeObject  segShape;
}_VGPathSegInfo;
ARRAY_DEFINE(_VGPathSegInfo);

/* temp _VGVertex struct for thirdparty */
typedef struct _VGVertex
{
    _VGVector2  point;
} _VGVertex;

typedef struct _TreeNode
{
    _VGuint8    type;       /* 0 = region, 1 = vertex, 2 = edge */
    gctINT32    objectIndex;
    gctINT32    leftBranch, rightBranch;
}_VGTreeNode;

typedef struct _Trapezoid
{
    gctINT32    treeNode;
    gctINT32    upperVertex;
    gctINT32    lowerVertex;
    gctINT32    leftEdge;
    gctINT32    rightEdge;
    gctBOOL     degen;
}_VGTrapezoid;

/* For tessellation module. */
struct _VGTessellationContext
{
#if LOCAL_MEM_OPTIM
    gcsMEM_AFS_MEM_POOL IntPool;
    gcsMEM_AFS_MEM_POOL IntpPool;
#endif
    gctFLOAT        strokeScale;
    gctFLOAT        strokeWidth;
    gctFLOAT        strokeError;
    gctINT32        strokeJoinStep;     /* For construct stroke join and caps. */
    /* Flattening Global State Variables. */
    _VGTesstype     iterations[9];
    _VGTesstype     lowAngle, highAngle;
    _VGTessPoint    segments[8193];
    gctINT32        direction;
    gctINT32        segCount;
    gctINT32        arcType;
    gctINT32        lowPos, highPos;

    /* Triangulation Global State Variables.*/
    gctINT32        **mountains;
    _VGVector2      *vertices;
    gctINT32        verticesLength;                       /* size of the array */
    gctINT32        *edgeHigh, *edgeLow;       /* high and low ends of an edge */
    gctINT32        numLoops;
    gctINT32        *loopStart;
    _VGTreeNode     *tree;
    gctINT32        treeLength;                           /* size of the array */
    gctINT32        treeTop;
    _VGTrapezoid    *regions;
    gctINT32        regionsLength;                        /* size of the array */
    gctINT32        totalPoints;
    gctINT32        originalTotalPoints;               /* for intersection */
    gctBOOL         *pointsAdded;
    gctBOOL         *edgeAdded;
    gctINT32        *pointsTreePosition;                         /* useful when point tracing is added */
    gctINT32        **edgeIn;
    gctINT32        edgeInLength;                         /* size of the array */
    gctINT32        *edgeInLengths;
    gctINT32        **edgeOut;
    gctINT32        edgeOutLength;                        /* size of the array */
    gctINT32        *edgeOutLengths;
    gctINT32        *edgeUpDown;
    gctINT32        regionCounter;
    gctINT32        *regionBelow;                      /* left region */
    gctINT32        *regionBelow2;                     /* right region */
    gctINT32        **regionBelow3;                    /* middle regions when 2 or more edges form a cusp */
    gctINT32        regionBelow3Length;                   /* size of the array*/
    gctINT32        *regionBelow3Lengths;
    gctINT32        **regionAbove;
    gctINT32        regionAboveLength;                    /* size of the array */
    gctINT32        *regionAboveLengths;
    gctINT32        *trapezoidDepth;                       /* number of layers of loops. Used for non-zero coloring */
    gctINT32        *nextEven;
    gctINT32        *nextOdd;
    gctINT32        *edgeDirection;
    _VGint16        *triangles;                       /* Return value to the caller, no destruction is needed. */
    gctINT32        triangleCounter;
    gctINT32        arcInside[2];
    gctBOOL         flattenForStroke;
    gctBOOL         errorHandling;
    _VGTesstype     epsilon;
    _VGTesstype     epsilonSquare;
    _VGTesstype     eBezierThold;
    _VGTesstype     angleEpsilon;
};

#if USE_FTA
struct _VGFlattenContext
{
    _VGTessPoint        **pointsInSegment;      /* An array of pointers. Which points to the result that the segment is flattened into. Remember to free before return. */
    gctINT32            *numPointsInSegment;    /* An array indicating how many points in a flattened segment. Remember to free before return. */
    gctINT32            *numSegsInSubPath;      /* An array indicating how many tContext->segments in a sub path. Remember to free before return. */
};
#endif

_VGTesstype _Diag(_VGTesstype x, _VGTesstype y);

/* ***********************. */
#define     MID_POINT(a, b, c)  \
    (c).x = TESS_MUL(((a).x + (b).x), TESS_CONST_HALF);  \
    (c).y = TESS_MUL(((a).y + (b).y), TESS_CONST_HALF);

#define     VECTOR_EQUAL(v0, v1) \
    (((v0).x == (v1.x)) && ((v0).y == (v1).y))

gctINT32    _BezierFlatten(
    _VGContext      *context,
    _VGVector2      cp[4],
    _VGTessPoint    **points
    );
void        _DivideBezier(
    _VGTessellationContext  *tContext,
    _VGVector2      p[],
    gctINT32        step,
    gctINT32        togo,
    gctINT32        checks,
    _VGTesstype     t0,
    _VGTesstype     t1
    );
gctINT32    _CreateEllipse(
    _VGTesstype     x,
    _VGTesstype     y,
    _VGTesstype     a,
    _VGVector2      *p1,
    _VGVector2      *p2,
    gctINT32        majorArc,
    gctBOOL         forcedSolution,
    _VGTesstype     *a1,
    _VGTesstype     *a2,
    _VGEllipse      *result
    );
gctINT32    _EllipseFlatten(
    _VGContext      *context,
    _VGEllipse      *aEllipse,
    _VGTesstype     startAngle,
    _VGTesstype     endAngle,
    _VGVector2      *startPoint,
    _VGVector2      *endPoint,
    _VGTessPoint    **result,
    gctINT32        offset
    );
void        _ReflectPoint(
    _VGTessPoint    *p,
    gctINT32        quadrant,
    _VGVector2      *center,
    _VGTesstype     c,
    _VGTesstype     s,
    _VGTessPoint    *result
    );
void        _DivideCircle(
    _VGTessellationContext  *tContext,
    _VGVector2      p[],
    gctINT32        i,
    _VGTesstype     pos,
    _VGTesstype     inc
    );
gctINT32    _StepsNeeded(
    _VGTessellationContext  *tContext,
    _VGVector2              p[]
    );
_VGTesstype _MockDistance(
    _VGVector2              p[]
    );
_VGTesstype _MockLog2(
    _VGTesstype x
    );
_VGTesstype _MockCubeRoot(
    _VGTesstype x
    );
/* Approximate a path by line segments, whose vertices are stored in "points". */
gctINT32    _FlattenPath(
    _VGContext          *context,
    _VGPath             *path,                    /* The Path to Process */
    _VGTesstype         strokeWidth,
    _VGFlattenBuffer    *flattened
    );

typedef _VGTesstype (*_ValueGetterFloat)(_VGint8 *data);

_VGTesstype _GetS8 (
    _VGint8 *data
    );
_VGTesstype _GetS16(
    _VGint8 *data
    );
_VGTesstype _GetS32(
    _VGint8 *data
    );
_VGTesstype _GetF  (
    _VGint8 *data
    );
void _ReversePointArray(
    _VGTessPoint    points[],
    gctINT32        length,
    gctBOOL         revTangent
    );

/* End of Fill Tessellation. */

/* Vincent: Stroke Tessellation. ***********************************************/
void    _ConstructStroke(
     _VGContext     *context,
     _VGTessPoint   *points,
     gctINT32       numPoints,
     gctBOOL        pathClosed,
     _StreamPipe    *streamPipe
    );
void    _TessellateFirstLine(
    _VGContext      *context,
    _VGTessPoint    linePoints[],
    gctBOOL         isLastLine,
    gctBOOL         isClosedPath,
    _StreamPipe     *streamPipe
    );
void    _TessellateLastLine(
    _VGContext      *context,
    _VGTessPoint    prevLinePoints[],
    _VGTessPoint    currLinePoints[],
    _VGTessPoint    firstLinePoints[],
    _StreamPipe     *streamPipe
    );
void    _TessellateMidLine(
    _VGContext      *context,
    _VGTessPoint    prevLinePoints[],
    _VGTessPoint    currLinePoints[],
    _StreamPipe     *streamPipe
    );

void    _ConstructStartCap(
    _VGContext      *context,
    _VGTessPoint    linePoints[],
    _StreamPipe     *streamPipe
    );
void    _ConstructStrokeBody(
    _VGContext      *context,
    _VGTesstype     strokeWidth,
    _VGTessPoint    linePoints[],
    _StreamPipe     *streamPipe
    );
void    _ConstructEndCap(
    _VGContext      *context,
    _VGTessPoint    linePoints[],
    _StreamPipe     *streamPipe
    );
void    _ConstructStrokeJoin(
    _VGContext      *context,
    _VGTessPoint    prevLinePoints[],
    _VGTessPoint    currLinePoints[],
    gctBOOL         closeJoin,
    _StreamPipe     *streamPipe
    );
gctINT32    _DivideIntoDashLines(
    _VGContext      *context,
    _VGTessPoint    points[],           /* Points of the flattened sub-path. */
    gctINT32        numPointsToDo,      /* The number of the points of the lines to process. */
    _VGTesstype     dashPattern[],      /* VG dash pattern definition. */
    gctBOOL         subPathClosed,
    gctINT32        dashCount,          /* VG dash count. */
    _VGTesstype     dashPhase,          /* VG dash phase. */
    _VGTesstype     *dashPhaseNext,     /* Dash phase for the next sub-path. If DASH_PHASE_RESET is TRUE, it is ignored. */
    gctINT32        *numDashSegs,
    _VGTessPoint    **resultPoints,     /* Points that the dahs generates */
    gctINT32        **countArray
    );      /* The number of points each continus segment contains. */
/* End of Stroke Tessellation. */

gceSTATUS        _Trapezoidation(
    _VGContext  *context
);
gctBOOL        _PointHigh(
    _VGTessellationContext  *tContext,
    gctINT32    p1,
    gctINT32    p2
    );
gctINT32    _PointHigh3Way(
    _VGTessellationContext  *tContext,
    gctINT32    p1,
    gctINT32    p2
    );
gceSTATUS        _AddEdge(
    _VGContext  *context,
    gctINT32    index
    );
gctINT32    _AddPoint(
    _VGContext  *context,
    gctINT32    index
    );
gctINT32    _PointLeftRight(
    _VGTessellationContext  *tContext,
    gctINT32    pointIndex,
    gctINT32    edgeIndex
    );
gctINT32    _EdgeLeftRight(
    _VGTessellationContext  *tContext,
    gctINT32    edge1,
    gctINT32    edge2
    );
gctINT32    _EdgeLeftRight3(
    _VGTessellationContext  *tContext,
    gctINT32    top,
    gctINT32    bottom1,
    gctINT32    bottom2
    );
gctINT32    *_SetTrapezoidDepth(
    _VGContext  *context,
    gctINT32    index,
    gctINT32    depth,
    gctINT32    *length
    );
gceSTATUS   _MakeMountains(
    _VGContext  *context,
    gctINT32    parity,
    gctINT32    *numMountains,
    gctINT32    **lengthArray
    );
gceSTATUS        _MoveMountain(
    _VGContext  *context,
    gctINT32    mountain[],
    gctINT32    mountainLength
    );
gctINT32    _CheckIntersection(
    _VGTessellationContext  *tContext,
    gctINT32    edge1,
    gctINT32    edge2,
    _VGVector2  *intersection
    );
gceSTATUS        _ResizeArrays(
    _VGContext  *context,
    gctINT32    newLength
    );
gceSTATUS        _BreakOneEdge2(
    _VGContext  *context,
    gctINT32    currRegion,
    gctINT32    theEdge,
    gctINT32    leftPointIndex,
    gctINT32    intersectionType
    );
void        _BreakOneEdge(
    _VGTessellationContext  *tContext,
    gctINT32    leftRegion,
    gctINT32    rightRegion,
    gctINT32    leftPointIndex
    );
gceSTATUS        _BreakEdge(
    _VGContext  *context,
    gctINT32    edgeToBreak,
    gctINT32    newEdgeIndex,
    gctINT32    leftRegion,
    gctINT32    rightRegion,
    gctINT32    leftPoint,
    gctINT32    rightPoint,
    gctINT32    breakType
    );
gctINT32    _FindOtherRegion(
    _VGTessellationContext  *tContext,
    gctINT32    thisRegion,
    gctINT32    thisPoint,
    gctINT32    direction
    );
gctINT32 TessellateStroke(
    _VGContext  *context,
    _VGPath     *path
    );
gctBOOL     Tessellate(
    _VGContext  *context,
    _VGPath     *path,
    _VGMatrix3x3 *matrix
    );
void GetPathBounds(
    _VGContext  *context,
    _VGPath     *p,
    VGfloat     *minx,
    VGfloat     *miny,
    VGfloat     *width,
    VGfloat     *height
    );
_VGTesstype GetPathLength(
    _VGPath     *p,
    VGint       start,
    VGint       num
    );
void    GetPointAlongPath(
    _VGPath     *p,
    gctINT32    startSegment,
    gctINT32    numSegments,
    _VGTesstype distance,
    _VGTesstype *x,
    _VGTesstype *y,
    _VGTesstype *tangentX,
    _VGTesstype *tangentY
    );
void    _SetEpsilonScale(
    _VGTessellationContext  *tContext,
    _VGTesstype  scale
    );
void    _SetStrokeFlatten(
    _VGTessellationContext  *tContext,
    _VGTesstype  stroke
    );

gceSTATUS _RearrangeEdges(
    _VGContext  *context,
    gctINT32    leftPoint,
    gctINT32    rightPoint,
    gctINT32    newPoint
    );
void _RearrangeOneEdge(
    _VGTessellationContext  *tContext,
    gctINT32 edge,
    gctINT32 point,
    gctINT32 otherEnd
    );
gctBOOL _EdgeInserted(
    _VGTessellationContext  *tContext,
    gctINT32 edge
    );
gctINT32 _FindUpperLeftRegion(
    _VGTessellationContext  *tContext,
    gctINT32 edge,
    gctINT32 point
    );
gctINT32 _FindUpperRightRegion(
    _VGTessellationContext  *tContext,
    gctINT32 edge,
    gctINT32 point
    );
gctINT32 *_FindOtherRegions(
    _VGContext  *context,
    gctINT32 thisRegion,
    gctINT32 thisPoint,
    gctINT32 direction,
    gctINT32 above,
    gctINT32 *arrayLength
    );
gceSTATUS _BreakEdgeBunch(
    _VGContext  *context,
    gctINT32    *regionList,
    gctINT32    regionListLength,
    gctINT32    leftPointIndex,
    gctINT32    edgeOffset
    );
gctINT32 _PointLeftRight2(
    _VGTessellationContext  *tContext,
    gctINT32 pointIndex,
    gctINT32 edgeIndex
    );

void _ResetFlattenStates(
    _VGTessellationContext  *tContext
);
void _ResetTriangulateStates(
    _VGContext  *context
);

gctINT32    _GetRadAngle(
    _VGTesstype unitx,
    _VGTesstype unity
    );

gctINT32    _GetCirclePointCount(
    gctINT32    angle0,     /* starting angle, By degree */
    gctINT32    angle1,     /* ending angle, By degree */
    gctINT32    step
    );
void    _FlattenCircle(
    _VGVector2 *center,
    gctFLOAT    radius,
    gctINT32    sp,
    gctINT32    ep,
    gctINT32    step,
    _VGVector2  *points
    );

void    _ExpandPipe(
                    _VGContext  *context,
                    _StreamPipe *pipe,
                    int          vertexMin,
                    int          indexMin
                    );

#ifdef OVG_PROFILER
#define BEGIN_PROFILE(tag)  \
    {   \
        const char *name = tag; \
        int time0 = GetTime();

#define END_PROFILE \
        time0 = GetTime() - time0;  \
        gcmTRACE(gcvLEVEL_VERBOSE, "%s %d ms", name, time0);    \
    }

#else
#define BEGIN_PROFILE(name)
#define END_PROFILE
#endif

#if USE_TA
#define GET_TASIZE(pointer) \
    (*((int*)((char*)(pointer) - 4)))
/* Triangulation Specific Memory Operations. */
void    *TA_Init(gcoOS os, int size, int levels);
gceSTATUS   TA_Malloc(gcoOS os, void **pointer, int size, int levels);
gceSTATUS   TA_Resize(gcoOS os, void **pointer, int newsize, int levels);
void    TA_Free(gcoOS os, void **pointer);
void    TA_Destroy(gcoOS os, void **pointer, int levels);
#endif

#endif /* __gc_vgsh_tessellator_h_ */

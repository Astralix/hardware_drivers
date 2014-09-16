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


#ifndef __gc_glff_profiler_h_
#define __gc_glff_profiler_h_

#ifdef __cplusplus
extern "C" {
#endif

#define GLES1_APICALLBASE 100
#define GLES1_ACTIVETEXTURE     100
#define GLES1_ALPHAFUNC     101
#define GLES1_ALPHAFUNCX    102
#define GLES1_BINDBUFFER    103
#define GLES1_BINDTEXTURE   104
#define GLES1_BLENDFUNC     105
#define GLES1_BUFFERDATA    106
#define GLES1_BUFFERSUBDATA     107
#define GLES1_CLEAR     108
#define GLES1_CLEARCOLOR    109
#define GLES1_CLEARCOLORX   110
#define GLES1_CLEARDEPTHF   111
#define GLES1_CLEARDEPTHX   112
#define GLES1_CLEARSTENCIL  113
#define GLES1_CLIENTACTIVETEXTURE   114
#define GLES1_CLIPPLANEF    115
#define GLES1_CLIPPLANEX    116
#define GLES1_COLOR4F   117
#define GLES1_COLOR4UB  118
#define GLES1_COLOR4X   119
#define GLES1_COLORMASK     120
#define GLES1_COLORPOINTER  121
#define GLES1_COMPRESSEDTEXIMAGE2D  122
#define GLES1_COMPRESSEDTEXSUBIMAGE2D   123
#define GLES1_COPYTEXIMAGE2D    124
#define GLES1_COPYTEXSUBIMAGE2D     125
#define GLES1_CULLFACE  126
#define GLES1_DELETEBUFFERS     127
#define GLES1_DELETETEXTURES    128
#define GLES1_DEPTHFUNC     129
#define GLES1_DEPTHMASK     130
#define GLES1_DEPTHRANGEF   131
#define GLES1_DEPTHRANGEX   132
#define GLES1_DISABLE   133
#define GLES1_DISABLECLIENTSTATE    134
#define GLES1_DRAWARRAYS    135
#define GLES1_DRAWELEMENTS  136
#define GLES1_ENABLE    137
#define GLES1_ENABLECLIENTSTATE     138
#define GLES1_FINISH    139
#define GLES1_FLUSH     140
#define GLES1_FOGF  141
#define GLES1_FOGFV     142
#define GLES1_FOGX  143
#define GLES1_FOGXV     144
#define GLES1_FRONTFACE     145
#define GLES1_FRUSTUMF  146
#define GLES1_FRUSTUMX  147
#define GLES1_GENBUFFERS    148
#define GLES1_GENTEXTURES   149
#define GLES1_GETBOOLEANV   150
#define GLES1_GETBUFFERPARAMETERIV  151
#define GLES1_GETCLIPPLANEF     152
#define GLES1_GETCLIPPLANEX     153
#define GLES1_GETERROR  154
#define GLES1_GETFIXEDV     155
#define GLES1_GETFLOATV     156
#define GLES1_GETINTEGERV   157
#define GLES1_GETLIGHTFV    158
#define GLES1_GETLIGHTXV    159
#define GLES1_GETMATERIALFV     160
#define GLES1_GETMATERIALXV     161
#define GLES1_GETPOINTERV   162
#define GLES1_GETSTRING     163
#define GLES1_GETTEXENVFV   164
#define GLES1_GETTEXENVIV   165
#define GLES1_GETTEXENVXV   166
#define GLES1_GETTEXPARAMETERFV     167
#define GLES1_GETTEXPARAMETERIV     168
#define GLES1_GETTEXPARAMETERXV     169
#define GLES1_HINT  170
#define GLES1_ISBUFFER  171
#define GLES1_ISENABLED     172
#define GLES1_ISTEXTURE     173
#define GLES1_LIGHTF    174
#define GLES1_LIGHTFV   175
#define GLES1_LIGHTMODELF   176
#define GLES1_LIGHTMODELFV  177
#define GLES1_LIGHTMODELX   178
#define GLES1_LIGHTMODELXV  179
#define GLES1_LIGHTX    180
#define GLES1_LIGHTXV   181
#define GLES1_LINEWIDTH     182
#define GLES1_LINEWIDTHX    183
#define GLES1_LOADIDENTITY  184
#define GLES1_LOADMATRIXF   185
#define GLES1_LOADMATRIXX   186
#define GLES1_LOGICOP   187
#define GLES1_MATERIALF     188
#define GLES1_MATERIALFV    189
#define GLES1_MATERIALX     190
#define GLES1_MATERIALXV    191
#define GLES1_MATRIXMODE    192
#define GLES1_MULTITEXCOORD4F   193
#define GLES1_MULTITEXCOORD4X   194
#define GLES1_MULTMATRIXF   195
#define GLES1_MULTMATRIXX   196
#define GLES1_NORMAL3F  197
#define GLES1_NORMAL3X  198
#define GLES1_NORMALPOINTER     199
#define GLES1_ORTHOF    200
#define GLES1_ORTHOX    201
#define GLES1_PIXELSTOREI   202
#define GLES1_POINTPARAMETERF   203
#define GLES1_POINTPARAMETERFV  204
#define GLES1_POINTPARAMETERX   205
#define GLES1_POINTPARAMETERXV  206
#define GLES1_POINTSIZE     207
#define GLES1_POINTSIZEX    208
#define GLES1_POLYGONOFFSET     209
#define GLES1_POLYGONOFFSETX    210
#define GLES1_POPMATRIX     211
#define GLES1_PUSHMATRIX    212
#define GLES1_READPIXELS    213
#define GLES1_ROTATEF   214
#define GLES1_ROTATEX   215
#define GLES1_SAMPLECOVERAGE    216
#define GLES1_SAMPLECOVERAGEX   217
#define GLES1_SCALEF    218
#define GLES1_SCALEX    219
#define GLES1_SCISSOR   220
#define GLES1_SHADEMODEL    221
#define GLES1_STENCILFUNC   222
#define GLES1_STENCILMASK   223
#define GLES1_STENCILOP     224
#define GLES1_TEXCOORDPOINTER   225
#define GLES1_TEXENVF   226
#define GLES1_TEXENVFV  227
#define GLES1_TEXENVI   228
#define GLES1_TEXENVIV  229
#define GLES1_TEXENVX   230
#define GLES1_TEXENVXV  231
#define GLES1_TEXIMAGE2D    232
#define GLES1_TEXPARAMETERF     233
#define GLES1_TEXPARAMETERFV    234
#define GLES1_TEXPARAMETERI     235
#define GLES1_TEXPARAMETERIV    236
#define GLES1_TEXPARAMETERX     237
#define GLES1_TEXPARAMETERXV    238
#define GLES1_TEXSUBIMAGE2D     239
#define GLES1_TRANSLATEF    240
#define GLES1_TRANSLATEX    241
#define GLES1_VERTEXPOINTER     242
#define GLES1_VIEWPORT  243
#define GLES1_BLENDEQUATIONOES         244
#define GLES1_BLENDFUNCSEPERATEOES     245
#define GLES1_BLENDEQUATIONSEPARATEOES 246
#define GLES1_GLMAPBUFFEROES 247
#define GLES1_GLUNMAPBUFFEROES 248
#define GLES1_GLGETBUFFERPOINTERVOES 249

#define GLES1_NUM_API_CALLS (GLES1_GLGETBUFFERPOINTERVOES - GLES1_APICALLBASE + 1)


#define GL_PROFILER_FRAME_END 10
#define GL_PROFILER_FRAME_TYPE 11
#define GL_PROFILER_FRAME_COUNT 12

#define GL1_PROFILER_PRIMITIVE_END 20
#define GL1_PROFILER_PRIMITIVE_TYPE 21
#define GL1_PROFILER_PRIMITIVE_COUNT 22

#define GL_TEXUPLOAD_SIZE 30


/* Profile information. */
typedef struct _glsPROFILER
{
    gctBOOL         enable;
    gctBOOL         drvEnable;
    gctBOOL         timeEnable;
    gctBOOL         memEnable;

    gctUINT32       frameBegun;

    /* Aggregate Information */
    gctUINT64       frameStart;
    gctUINT64       frameEnd;

    /* Current frame information */
    gctUINT32       frameNumber;
    gctUINT64       frameStartTimeusec;
    gctUINT64       frameEndTimeusec;
    gctUINT64       frameStartCPUTimeusec;
    gctUINT64       frameEndCPUTimeusec;

    gctUINT64       shaderCompileTime;
    gctUINT64       shaderStartTimeusec;
    gctUINT64       shaderEndTimeusec;

    gctUINT32       drawPointCount_11;
    gctUINT32       drawLineCount_11;
    gctUINT32       drawTriangleCount_11;
    gctUINT32       drawVertexCount_11;

    /* Current primitive information */
    gctUINT32       primitiveNumber;
    gctUINT64       primitiveStartTimeusec;
    gctUINT64       primitiveEndTimeusec;
    gctUINT32       primitiveType;
    gctUINT32       primitiveCount;

    gctUINT32       apiCalls[GLES1_NUM_API_CALLS];
    gctUINT32       redundantStateChangeCalls;

	gctUINT64       apiTimes[GLES1_NUM_API_CALLS];
	gctUINT64       totalDriverTime;

    gctUINT32       textureUploadSize;
}
glsPROFILER;

void
_glffInitializeProfiler(
    glsCONTEXT_PTR Context
    );

void
_glffDestroyProfiler(
    glsCONTEXT_PTR Context
    );

void
_glffProfiler(
    gctPOINTER Profiler,
    gctUINT32 Enum,
    gctUINT32 Value
    );

#if VIVANTE_PROFILER
#       define glmPROFILE(c, e, v) \
        do \
        { \
                if (e >= 100) \
                { \
                        c->profiler.apiCalls[e - 100]++; \
						funcIndex = e; \
                } \
                else \
                { \
                        _glffProfiler(&c->profiler, e, v); \
                } \
        } \
        while (gcvFALSE)

#       define glmGetApiTime(c, e) \
	    do \
         { \
                 if (e >= 100) \
                 { \
                         c->profiler.apiTimes[e - 100]+=(endTimeusec-startTimeusec); \
                 } \
         } \
         while (gcvFALSE)

#        define glmDeclareTimeVar() \
         gctUINT64 startTimeusec, endTimeusec; \
		 gctUINT32 funcIndex = 0;

#        define glmGetApiStartTime() \
		 do \
         { \
	 	     gcoOS_GetTime(&startTimeusec); \
         } \
		 while (gcvFALSE)

#        define glmGetApiEndTime(c) \
	 	 do \
         { \
     	     gcoOS_GetTime(&endTimeusec); \
		     c->profiler.totalDriverTime+=(endTimeusec-startTimeusec); \
			 glmGetApiTime(c, funcIndex); \
         } \
         while (gcvFALSE)

#else
#       define glmPROFILE(c, e, v) do { } while (gcvFALSE)
#       define glmGetApiTime(c, e) do { } while (gcvFALSE)
#       define glmGetApiStartTime() do { } while (gcvFALSE)
#       define glmGetApiEndTime(c)  do { } while (gcvFALSE)
#       define glmDeclareTimeVar() \

#endif


#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_profiler_h_ */

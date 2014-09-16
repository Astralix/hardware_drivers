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


#ifndef __gc_glsh_profiler_h_
#define __gc_glsh_profiler_h_

#define GLES2_APICALLBASE 100
#define GLES2_ACTIVETEXTURE 100
#define GLES2_ATTACHSHADER 101
#define GLES2_BINDATTRIBLOCATION 102
#define GLES2_BINDBUFFER 103
#define GLES2_BINDFRAMEBUFFER 104
#define GLES2_BINDRENDERBUFFER 105
#define GLES2_BINDTEXTURE 106
#define GLES2_BLENDCOLOR 107
#define GLES2_BLENDEQUATION 108
#define GLES2_BLENDEQUATIONSEPARATE 109
#define GLES2_BLENDFUNC 110
#define GLES2_BLENDFUNCSEPARATE 111
#define GLES2_BUFFERDATA 112
#define GLES2_BUFFERSUBDATA 113
#define GLES2_CHECKFRAMEBUFFERSTATUS 114
#define GLES2_CLEAR 115
#define GLES2_CLEARCOLOR 116
#define GLES2_CLEARDEPTHF 117
#define GLES2_CLEARSTENCIL 118
#define GLES2_COLORMASK 119
#define GLES2_COMPILESHADER 120
#define GLES2_COMPRESSEDTEXIMAGE2D 121
#define GLES2_COMPRESSEDTEXSUBIMAGE2D 122
#define GLES2_COPYTEXIMAGE2D 123
#define GLES2_COPYTEXSUBIMAGE2D 124
#define GLES2_CREATEPROGRAM 125
#define GLES2_CREATESHADER 126
#define GLES2_CULLFACE 127
#define GLES2_DELETEBUFFERS 128
#define GLES2_DELETEFRAMEBUFFERS 129
#define GLES2_DELETEPROGRAM 130
#define GLES2_DELETERENDERBUFFERS 131
#define GLES2_DELETESHADER 132
#define GLES2_DELETETEXTURES 133
#define GLES2_DEPTHFUNC 134
#define GLES2_DEPTHMASK 135
#define GLES2_DEPTHRANGEF 136
#define GLES2_DETACHSHADER 137
#define GLES2_DISABLE 138
#define GLES2_DISABLEVERTEXATTRIBARRAY 139
#define GLES2_DRAWARRAYS 140
#define GLES2_DRAWELEMENTS 141
#define GLES2_ENABLE 142
#define GLES2_ENABLEVERTEXATTRIBARRAY 143
#define GLES2_FINISH 144
#define GLES2_FLUSH 145
#define GLES2_FRAMEBUFFERRENDERBUFFER 146
#define GLES2_FRAMEBUFFERTEXTURE2D 147
#define GLES2_FRONTFACE 148
#define GLES2_GENBUFFERS 149
#define GLES2_GENERATEMIPMAP 150
#define GLES2_GENFRAMEBUFFERS 151
#define GLES2_GENRENDERBUFFERS 152
#define GLES2_GENTEXTURES 153
#define GLES2_GETACTIVEATTRIB 154
#define GLES2_GETACTIVEUNIFORM 155
#define GLES2_GETATTACHEDSHADERS 156
#define GLES2_GETATTRIBLOCATION 157
#define GLES2_GETBOOLEANV 158
#define GLES2_GETBUFFERPARAMETERIV 159
#define GLES2_GETERROR 160
#define GLES2_GETFLOATV 161
#define GLES2_GETFRAMEBUFFERATTACHMENTPARAMETERIV 162
#define GLES2_GETINTEGERV 163
#define GLES2_GETPROGRAMIV 164
#define GLES2_GETPROGRAMINFOLOG 165
#define GLES2_GETRENDERBUFFERPARAMETERIV 166
#define GLES2_GETSHADERIV 167
#define GLES2_GETSHADERINFOLOG 168
#define GLES2_GETSHADERPRECISIONFORMAT 169
#define GLES2_GETSHADERSOURCE 170
#define GLES2_GETSTRING 171
#define GLES2_GETTEXPARAMETERFV 172
#define GLES2_GETTEXPARAMETERIV 173
#define GLES2_GETUNIFORMFV 174
#define GLES2_GETUNIFORMIV 175
#define GLES2_GETUNIFORMLOCATION 176
#define GLES2_GETVERTEXATTRIBFV 177
#define GLES2_GETVERTEXATTRIBIV 178
#define GLES2_GETVERTEXATTRIBPOINTERV 179
#define GLES2_HINT 180
#define GLES2_ISBUFFER 181
#define GLES2_ISENABLED 182
#define GLES2_ISFRAMEBUFFER 183
#define GLES2_ISPROGRAM 184
#define GLES2_ISRENDERBUFFER 185
#define GLES2_ISSHADER 186
#define GLES2_ISTEXTURE 187
#define GLES2_LINEWIDTH 188
#define GLES2_LINKPROGRAM 189
#define GLES2_PIXELSTOREI 190
#define GLES2_POLYGONOFFSET 191
#define GLES2_READPIXELS 192
#define GLES2_RELEASESHADERCOMPILER 193
#define GLES2_RENDERBUFFERSTORAGE 194
#define GLES2_SAMPLECOVERAGE 195
#define GLES2_SCISSOR 196
#define GLES2_SHADERBINARY 197
#define GLES2_SHADERSOURCE 198
#define GLES2_STENCILFUNC 199
#define GLES2_STENCILFUNCSEPARATE 200
#define GLES2_STENCILMASK 201
#define GLES2_STENCILMASKSEPARATE 202
#define GLES2_STENCILOP 203
#define GLES2_STENCILOPSEPARATE 204
#define GLES2_TEXIMAGE2D 205
#define GLES2_TEXPARAMETERF 206
#define GLES2_TEXPARAMETERFV 207
#define GLES2_TEXPARAMETERI 208
#define GLES2_TEXPARAMETERIV 209
#define GLES2_TEXSUBIMAGE2D 210
#define GLES2_UNIFORM1F 211
#define GLES2_UNIFORM1FV 212
#define GLES2_UNIFORM1I 213
#define GLES2_UNIFORM1IV 214
#define GLES2_UNIFORM2F 215
#define GLES2_UNIFORM2FV 216
#define GLES2_UNIFORM2I 217
#define GLES2_UNIFORM2IV 218
#define GLES2_UNIFORM3F 219
#define GLES2_UNIFORM3FV 220
#define GLES2_UNIFORM3I 221
#define GLES2_UNIFORM3IV 222
#define GLES2_UNIFORM4F 223
#define GLES2_UNIFORM4FV 224
#define GLES2_UNIFORM4I 225
#define GLES2_UNIFORM4IV 226
#define GLES2_UNIFORMMATRIX2FV 227
#define GLES2_UNIFORMMATRIX3FV 228
#define GLES2_UNIFORMMATRIX4FV 229
#define GLES2_USEPROGRAM 230
#define GLES2_VALIDATEPROGRAM 231
#define GLES2_VERTEXATTRIB1F 232
#define GLES2_VERTEXATTRIB1FV 233
#define GLES2_VERTEXATTRIB2F 234
#define GLES2_VERTEXATTRIB2FV 235
#define GLES2_VERTEXATTRIB3F 236
#define GLES2_VERTEXATTRIB3FV 237
#define GLES2_VERTEXATTRIB4F 238
#define GLES2_VERTEXATTRIB4FV 239
#define GLES2_VERTEXATTRIBPOINTER 240
#define GLES2_VIEWPORT 241
#define GLES2_GETPROGRAMBINARYOES 242
#define GLES2_PROGRAMBINARYOES 243
#define GLES2_TEXIMAGE3DOES 244
#define GLES2_TEXSUBIMAGE3DOES 245
#define GLES2_COPYSUBIMAGE3DOES 246
#define GLES2_COMPRESSEDTEXIMAGE3DOES 247
#define GLES2_COMPRESSEDTEXSUBIMAGE3DOES 248
#define GLES2_FRAMEBUFFERTEXTURE3DOES 249
#define GLES2_BINDVERTEXARRAYOES 250
#define GLES2_GENVERTEXARRAYOES 251
#define GLES2_ISVERTEXARRAYOES 252
#define GLES2_DELETEVERTEXARRAYOES 253
#define GLES2_GLMAPBUFFEROES 254
#define GLES2_GLUNMAPBUFFEROES 255
#define GLES2_GLGETBUFFERPOINTERVOES 256
#define GLES2_DISCARDFRAMEBUFFEREXT 257
#define GLES2_NUM_API_CALLS 258

#define GL2_PROFILER_FRAME_END 10
#define GL2_PROFILER_FRAME_TYPE 11
#define GL2_PROFILER_FRAME_COUNT 12

#define GL2_PROFILER_PRIMITIVE_END 20
#define GL2_PROFILER_PRIMITIVE_TYPE 21
#define GL2_PROFILER_PRIMITIVE_COUNT 22

#define GL2_TEXUPLOAD_SIZE 30

#define GL_COMPILER_SHADER_LENGTH 40
#define GL_COMPILER_START_TIME 41
#define GL_COMPILER_END_TIME 42

#define GL_SHADER_OBJECT 50

#define GL_PROGRAM_IN_USE_BEGIN 60
#define GL_PROGRAM_IN_USE_END 61
#define GL_PROGRAM_ATTRIB_CNT 62
#define GL_PROGRAM_UNIFORM_CNT 63
#define GL_PROGRAM_VERTEX_SHADER 64
#define GL_PROGRAM_FRAGMENT_SHADER 65

/* Profile information. */
typedef struct _glsPROFILER
{
    gctBOOL         enable;
        gctBOOL                 timeEnable;
        gctBOOL                 drvEnable;
        gctBOOL                 memEnable;
        gctBOOL                 progEnable;

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

    gctUINT32       drawPointCount;
    gctUINT32       drawLineCount;
    gctUINT32       drawTriangleCount;
    gctUINT32       drawVertexCount;

    /* Current primitive information */
    gctUINT32       primitiveNumber;
    gctUINT64       primitiveStartTimeusec;
    gctUINT64       primitiveEndTimeusec;
    gctUINT32       primitiveType;
    gctUINT32       primitiveCount;


    gctUINT32       apiCalls[GLES2_NUM_API_CALLS];
    gctUINT32       redundantStateChangeCalls;

	gctUINT64       apiTimes[GLES2_NUM_API_CALLS];
 	gctUINT64       totalDriverTime;

    gctUINT32       textureUploadSize;
}
glsPROFILER;

void
_glshProfiler(
    gctPOINTER Profiler,
    gctUINT32 Enum,
    gctUINT32 Value
    );

#if VIVANTE_PROFILER
#	define glmPROFILE(c, e, v) \
	do \
	{ \
		if (e >= 100) \
		{ \
			c->profiler.apiCalls[e - 100]++; \
		} \
		else \
		{ \
			_glshProfiler(&c->profiler, e, v); \
		} \
	} \
	while (gcvFALSE)

#   define glmGetApiTime(c, e) \
 	    do \
        { \
            if (e >= 100) \
            { \
			    c->profiler.apiTimes[e - 100]+=(endTimeusec-startTimeusec); \
            } \
         } \
         while (gcvFALSE)

#    define glmDeclareTimeVar() \
          gctUINT64 startTimeusec, endTimeusec;

#    define glmGetApiStartTime() \
		  do \
          { \
	 	      gcoOS_GetTime(&startTimeusec); \
          } \
		  while (gcvFALSE)

#    define glmGetApiEndTime(c) \
	 	  do \
          { \
     		  gcoOS_GetTime(&endTimeusec); \
			  c->profiler.totalDriverTime+=(endTimeusec-startTimeusec); \
          } \
          while (gcvFALSE)

#else
#	define glmPROFILE(c, e, v) do { } while (gcvFALSE)
#   define glmGetApiTime(c, e) do { } while (gcvFALSE)
#   define glmGetApiStartTime() do { } while (gcvFALSE)
#   define glmGetApiEndTime(c)  do { } while (gcvFALSE)
#   define glmDeclareTimeVar() \

#endif

#endif /* __gc_glsh_profiler_h_ */

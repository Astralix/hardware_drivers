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


#ifndef __gc_vgsh_profiler_h_
#define	__gc_vgsh_profiler_h_

#define APICALLBASE					100
#define VGAPPENDPATH				100
#define VGAPPENDPATHDATA			101
#define VGCHILDIMAGE 				102
#define VGCLEAR 					103
#define VGCLEARGLYPH 				104
#define VGCLEARIMAGE 				105
#define VGCLEARPATH 				106
#define VGCOLORMATRIX 				107
#define VGCONVOLVE					108
#define VGCOPYIMAGE					109
#define VGCOPYMASK 					110
#define VGCOPYPIXELS 				111
#define VGCREATEFONT 				112
#define VGCREATEIMAGE 				113
#define VGCREATEMASKLAYER 			114
#define VGCREATEPAINT 				115
#define VGCREATEPATH 				116
#define VGDESTROYFONT 				117
#define VGDESTROYIMAGE 				118
#define VGDESTROYMASKLAYER 			119
#define VGDESTROYPAINT 				120
#define VGDESTROYPATH 				121
#define VGDRAWGLYPH 				122
#define VGDRAWGLYPHS 				123
#define VGDRAWIMAGE 				124
#define VGDRAWPATH					125
#define VGFILLMASKLAYER 			126
#define VGFINISH 					127
#define VGFLUSH 					128
#define VGGAUSSIANBLUR 				129
#define VGGETCOLOR 					130
#define VGGETERROR					131
#define VGGETF 						132
#define VGGETFV 					133
#define VGGETI						134
#define VGGETIMAGESUBDATA 			135
#define VGGETIV 					136
#define VGGETMATRIX 				137
#define VGGETPAINT 					138
#define VGGETPARAMETERF 			139
#define VGGETPARAMETERFV			140
#define VGGETPARAMETERI 			141
#define VGGETPARAMETERIV 			142
#define VGGETPARAMETERVECTORSIZE 	143
#define VGGETPARENT 				144
#define VGGETPATHCAPABILITIES 		145
#define VGGETPIXELS 				146
#define VGGETSTRING 				147
#define VGGETVECTORSIZE 			148
#define VGHARDWAREQUERY 			149
#define VGIMAGESUBDATA 				150
#define VGINTERPOLATEPATH			151
#define VGLOADIDENTITY 				152
#define VGLOADMATRIX 				153
#define VGLOOKUP 					154
#define VGLOOKUPSINGLE				155
#define VGMASK 						156
#define VGMODIFYPATHCOORDS 			157
#define VGMULTMATRIX 				158
#define VGPAINTPATTERN 				159
#define VGPATHBOUNDS 				160
#define VGPATHLENGTH 				161
#define VGPATHTRANSFORMEDBOUNDS 	162
#define VGPOINTALONGPATH 			163
#define VGREADPIXELS 				164
#define VGREMOVEPATHCAPABILITIES 	165
#define VGRENDERTOMASK 				166
#define VGROTATE 					167
#define VGSCALE 					168
#define VGSEPARABLECONVOLVE 		169
#define VGSETCOLOR 					170
#define VGSETF 						171
#define VGSETFV 					172
#define VGSETGLYPHTOIMAGE 			173
#define VGSETGLYPHTOPATH 			174
#define VGSETI 						175
#define VGSETIV 					176
#define VGSETPAINT 					177
#define VGSETPARAMETERF 			178
#define VGSETPARAMETERFV 			179
#define VGSETPARAMETERI 			180
#define VGSETPARAMETERIV 			181
#define VGSETPIXELS 				182
#define VGSHEAR 					183
#define VGTRANSFORMPATH 			184
#define VGTRANSLATE 				185
#define VGWRITEPIXELS 				186

#define NUM_API_CALLS 87


#define VG_PROFILER_FRAME_END 10
#define VG_PROFILER_FRAME_TYPE 11
#define VG_PROFILER_FRAME_COUNT 12

#define VG_PROFILER_PRIMITIVE_END 20
#define VG_PROFILER_PRIMITIVE_TYPE 21
#define VG_PROFILER_PRIMITIVE_COUNT 22
#define	VG_PROFILER_STROKE	23
#define	VG_PROFILER_FILL	24

#define	VG_PATH		2000
#define	VG_GLYPH	2001
#define	VG_IMAGE	2002

/* Profile information. */
struct _VGProfile
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

    gctUINT32       drawPathCount;
	gctUINT32		drawGlyphCount;
    gctUINT32       drawImageCount;
	gctUINT32		drawFillCount;
	gctUINT32		drawStrokeCount;

    /* Current primitive information */
    gctUINT32       primitiveNumber;
    gctUINT64       primitiveStartTimeusec;
    gctUINT64       primitiveEndTimeusec;
    gctUINT32       primitiveType;
    gctUINT32       primitiveCount;

    gctUINT32       apiCalls[NUM_API_CALLS];
    gctUINT32       redundantStateChangeCalls;

    gctUINT32       imageUploadSize;
};

#if VIVANTE_PROFILER
void
InitializeVGProfiler(
    _VGContext * Context
    );

void DestroyVGProfiler(
    _VGContext * Context
    );

void
vgProfiler(
	gctPOINTER Profiler,
	gctUINT32 Enum,
	gctUINT32 Value
    );

#define	vgmPROFILE(context, type, value) \
	do\
	{ \
		if ((type - APICALLBASE < NUM_API_CALLS) \
		&&	(type >= APICALLBASE) \
		) \
		{ \
			context->profiler.apiCalls[type - APICALLBASE]++; \
		} \
		else \
		{ \
			vgProfiler(&context->profiler, type, value); \
		} \
	} \
	while(gcvFALSE)
#else
#define	vgmPROFILE(context, type, value) do { } while (0)
#endif

#endif /* __gc_vgsh_profiler_h_ */

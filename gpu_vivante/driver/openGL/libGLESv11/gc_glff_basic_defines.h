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




#ifndef __gc_glff_basic_defines_h_
#define __gc_glff_basic_defines_h_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
********************** Function parameter dumping macros. **********************
\******************************************************************************/

#define glmARGINT   "%d"
#define glmARGUINT  "%u"
#define glmARGENUM  "0x%04X"
#define glmARGFIXED "0x%08X"
#define glmARGHEX   "0x%08X"
#define glmARGPTR   "0x%x"
#define glmARGFLOAT "%1.4f"

#define glmERROR(result) \
{ \
    GLenum lastResult = result;  \
    \
    if (glmIS_ERROR(lastResult)) \
    { \
        glsCONTEXT * _context = GetCurrentContext(); \
        gcmTRACE( \
            gcvLEVEL_ERROR, \
            "glmERROR: result=0x%04X @ %s(%d)", \
            lastResult, __FUNCTION__, __LINE__ \
            ); \
        \
        if (_context && glmIS_SUCCESS(_context->error)) \
        {  \
            _context->error = lastResult; \
        } \
    } \
} \
do { } while (gcvFALSE)


#define __glmDOENTER() \
    do \
    { \
        context = GetCurrentContext(); \
		glmGetApiStartTime(); \
        \
        if (context == gcvNULL) \
        { \
            break; \
        }

#define glmENTER() \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER(); \
    \
    __glmDOENTER()

#define glmENTER1( \
    Type, Value \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value "=" Type, Value); \
    \
    __glmDOENTER()

#define glmENTER2( \
    Type1, Value1, \
    Type2, Value2 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2, \
                  Value1, Value2); \
    \
    __glmDOENTER()

#define glmENTER3( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3, \
                  Value1, Value2, Value3); \
    \
    __glmDOENTER()

#define glmENTER4( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4, \
                  Value1, Value2, Value3, Value4); \
    \
    __glmDOENTER()

#define glmENTER5( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5, \
                  Value1, Value2, Value3, Value4, Value5); \
    \
    __glmDOENTER()

#define glmENTER6( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6, \
                  Value1, Value2, Value3, Value4, Value5, Value6); \
    \
    __glmDOENTER()

#define glmENTER7( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7); \
    \
    __glmDOENTER()

#define glmENTER8( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7, \
    Type8, Value8 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7 \
                  " " #Value8 "=" Type8, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7, \
                  Value8); \
    \
    __glmDOENTER()

#define glmENTER9( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7, \
    Type8, Value8, \
    Type9, Value9 \
    ) \
    glsCONTEXT_PTR context; \
	glmDeclareTimeVar() \
	\
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7 \
                  " " #Value8 "=" Type8 \
                  " " #Value9 "=" Type9, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7, \
                  Value8, Value9); \
    \
    __glmDOENTER()

#define glmLEAVE() \
    } \
    while (GL_FALSE); \
	glmGetApiEndTime(context);\
    gcmFOOTER_ARG("error=0x%04X", (context == gcvNULL) ? 0 \
                                                       : context->error) \


/******************************************************************************\
**************************** Error checking macros. ****************************
\******************************************************************************/

#define glmIS_ERROR(func) \
    ((func) != GL_NO_ERROR)

#define glmIS_SUCCESS(func) \
    ((func) == GL_NO_ERROR)

#define glmERR_BREAK(func) \
    result = func; \
    if (glmIS_ERROR(result)) \
    { \
        break; \
    }

#define glmTRANSLATEHALSTATUS(func) \
    ( \
        gcmIS_SUCCESS(func) \
            ? GL_NO_ERROR \
            : GL_INVALID_OPERATION \
    )

#define glmTRANSLATEGLRESULT(func) \
    ( \
        glmIS_SUCCESS(func) \
            ? gcvSTATUS_OK \
            : gcvSTATUS_GENERIC_IO \
    )


/******************************************************************************\
************************ Debug zones (28 are available). ***********************
\******************************************************************************/

#define glvZONE_BUFFER          (gcvZONE_API_ES11 | (1 <<  0))
#define glvZONE_CLEAR           (gcvZONE_API_ES11 | (1 <<  1))
#define glvZONE_CLIP            (gcvZONE_API_ES11 | (1 <<  2))
#define glvZONE_CONTEXT         (gcvZONE_API_ES11 | (1 <<  3))
#define glvZONE_DRAW            (gcvZONE_API_ES11 | (1 <<  4))
#define glvZONE_ENABLE          (gcvZONE_API_ES11 | (1 <<  5))
#define glvZONE_EXTENTION       (gcvZONE_API_ES11 | (1 <<  6))
#define glvZONE_FOG             (gcvZONE_API_ES11 | (1 <<  7))
#define glvZONE_FRAGMENT        (gcvZONE_API_ES11 | (1 <<  8))
#define glvZONE_LIGHT           (gcvZONE_API_ES11 | (1 <<  9))
#define glvZONE_MATRIX          (gcvZONE_API_ES11 | (1 << 10))
#define glvZONE_PIXEL           (gcvZONE_API_ES11 | (1 << 11))
#define glvZONE_POLIGON         (gcvZONE_API_ES11 | (1 << 12))
#define glvZONE_LINE            (gcvZONE_API_ES11 | (1 << 13))
#define glvZONE_QUERY           (gcvZONE_API_ES11 | (1 << 14))
#define glvZONE_TEXTURE         (gcvZONE_API_ES11 | (1 << 15))
#define glvZONE_STATES          (gcvZONE_API_ES11 | (1 << 16))
#define glvZONE_STREAM          (gcvZONE_API_ES11 | (1 << 17))
#define glvZONE_VIEWPORT        (gcvZONE_API_ES11 | (1 << 18))
#define glvZONE_SHADER          (gcvZONE_API_ES11 | (1 << 19))
#define glvZONE_HASH            (gcvZONE_API_ES11 | (1 << 20))
#define glvZONE_TRACE           (gcvZONE_API_ES11 | (1 << 21))

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_basic_defines_h_ */

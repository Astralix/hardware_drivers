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


#ifndef __gc_glsh_precompiled_shaer_objects_
#define __gc_glsh_precompiled_shaer_objects_

#include "gc_glsl_util.h"

#define EGYPT3 0
#define EGYPT4 0

static gceSTATUS
_Shader1(
    gcSHADER Shader
    )
{
    gcATTRIBUTE attribute;
    gcUNIFORM uniform;
    gceSTATUS status;

    gcmONERROR(gcSHADER_AddAttribute(Shader, "myVertex", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "myVertex1", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "myVertex2", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "myVertex3", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "myVertex4", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddUniform(Shader, "time", gcSHADER_FLOAT_X1, 1, &uniform));
    gcmONERROR(gcSHADER_AddOutput(Shader, "vVertex", gcSHADER_FLOAT_X4, 1, 0));
    gcmONERROR(gcSHADER_AddOutput(Shader, "vVertex1", gcSHADER_FLOAT_X4, 1, 1));
    gcmONERROR(gcSHADER_AddOutput(Shader, "vVertex2", gcSHADER_FLOAT_X4, 1, 2));
    gcmONERROR(gcSHADER_AddOutput(Shader, "vVertex3", gcSHADER_FLOAT_X4, 1, 3));
    gcmONERROR(gcSHADER_AddOutput(Shader, "vVertex4", gcSHADER_FLOAT_X4, 1, 4));
    gcmONERROR(gcSHADER_AddOutput(Shader, "#Position", gcSHADER_FLOAT_X4, 1, 155));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 0, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 1, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 1, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 2, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 2, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 3, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 3, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 4, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 6, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 7, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 6, 0x2, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 1, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 7, 0x2, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 1, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 6, 0x4, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 2, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 7, 0x4, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 2, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 6, 0x8, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 3, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 7, 0x8, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 3, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 11, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 6, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 12, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 11, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 13, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 7, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 14, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 12, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 13, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 16, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 14, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 15, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 16, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 20, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 6, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 21, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 20, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 23, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 21, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 21, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 24, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 21, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 21, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 25, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 24, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 17, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 23, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 25, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 29, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 7, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 30, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 29, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 32, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 30, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 30, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 33, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 30, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 30, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 34, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 33, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 26, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 32, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 34, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 35, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 17, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 26, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 36, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 35, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 37, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 6, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 38, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 37, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 40, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 36, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 15, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 41, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 38, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 40, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 10, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 15, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 41, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 43, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 44, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 43, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 45, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 46, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 44, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 45, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 48, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 46, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 47, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 48, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 52, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 53, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 52, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 55, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 53, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 53, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 56, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 53, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 53, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 57, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 56, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 49, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 55, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 57, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 61, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 62, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 61, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 64, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 62, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 62, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 65, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 62, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 62, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 66, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 65, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 58, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 64, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 66, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 67, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 49, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 58, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 68, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 67, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 69, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 70, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 69, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 72, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 68, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 47, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 73, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 70, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 72, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 42, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 47, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 73, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 75, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 7, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 76, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 75, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 77, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 6, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 78, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 76, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 77, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 80, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 78, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 79, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 80, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 84, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 7, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 85, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 84, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 87, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 85, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 85, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 88, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 85, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 85, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 89, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 88, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 81, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 87, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 89, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 93, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 6, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 94, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 93, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 96, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 94, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 94, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 97, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 94, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 94, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 98, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 97, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 90, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 96, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 98, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 99, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 81, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 90, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 100, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 99, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 101, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 7, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 102, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 101, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 104, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 100, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 79, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 105, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 102, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 104, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 74, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 79, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 105, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 107, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 108, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 107, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 109, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 110, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 108, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 109, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 112, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 110, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 111, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 112, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 116, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 117, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 116, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 119, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 117, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 117, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 120, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 117, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 117, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 121, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 120, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 113, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 119, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 121, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 125, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 126, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 125, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 128, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 126, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 126, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 129, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 126, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 126, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 130, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 129, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 122, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 128, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 130, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 131, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 113, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 122, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 132, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 131, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 133, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 134, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 133, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 136, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 132, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 111, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 137, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 134, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 136, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 106, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 111, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 137, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 138, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 10, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 10, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 139, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 138, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 10, 0xAA, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 140, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 139, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 10, 0xFF, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 141, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 140, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 42, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 142, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 141, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 74, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 143, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 142, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 74, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 144, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 143, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 74, 0xAA, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 145, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 144, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 74, 0xFF, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 146, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 145, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 106, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 147, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 146, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.100000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 5, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 147, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 149, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 150, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 149, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 151, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 152, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 151, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 153, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 150, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 152, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 154, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 5, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 153, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 156, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 154, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.333300f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 158, 0x8, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 156, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 158, 0x3, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 158, 0x4, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 154, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 155, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 158, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_NOP, 0, 0x0, gcSL_FLOAT));
    return gcvSTATUS_OK;

OnError:
    return status;
}

static gceSTATUS
_Shader2(
    gcSHADER Shader
    )
{
    gcATTRIBUTE attribute;
    gcUNIFORM uniform;
    gceSTATUS status;

    gcmONERROR(gcSHADER_AddAttribute(Shader, "vVertex", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "vVertex1", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "vVertex2", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "vVertex3", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddAttribute(Shader, "vVertex4", gcSHADER_FLOAT_X4, 1, gcvFALSE, &attribute));
    gcmONERROR(gcSHADER_AddUniform(Shader, "time", gcSHADER_FLOAT_X1, 1, &uniform));
    gcmONERROR(gcSHADER_AddOutput(Shader, "#Color", gcSHADER_FLOAT_X4, 1, 144));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 1, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 2, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 1, 0x2, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 1, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 2, 0x2, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 1, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 1, 0x4, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 2, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 2, 0x4, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 2, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 1, 0x8, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 3, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 2, 0x8, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 3, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 6, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 1, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 7, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 6, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 8, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 2, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 9, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 7, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 8, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 11, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 9, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 10, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 11, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 15, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 1, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 16, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 15, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 18, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 16, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 16, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 19, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 16, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 16, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 20, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 19, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 12, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 18, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 20, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 24, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 2, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 25, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 24, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 27, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 25, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 25, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 28, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 25, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 25, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 29, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 28, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 21, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 27, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 29, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 30, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 12, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 21, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 31, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 30, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 32, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 1, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 33, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 32, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 35, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 31, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 10, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 36, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 33, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 35, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 5, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 10, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 36, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 38, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 39, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 38, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 40, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 41, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 39, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 40, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 43, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 41, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 42, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 43, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 47, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 48, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 47, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 50, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 48, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 48, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 51, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 48, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 48, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 52, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 51, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 44, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 50, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 52, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 56, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 57, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 56, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 59, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 57, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 57, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 60, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 57, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 57, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 61, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 60, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 53, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 59, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 61, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 62, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 44, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 53, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 63, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 62, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 64, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 65, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 64, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 67, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 63, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 42, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 68, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 65, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 67, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 37, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 42, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 68, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 70, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 2, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 71, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 70, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 72, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 1, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 73, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 71, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 72, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 75, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 73, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 74, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 75, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 79, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 2, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 80, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 79, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 82, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 80, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 80, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 83, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 80, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 80, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 84, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 83, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 76, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 82, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 84, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 88, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 1, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 89, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 88, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 91, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 89, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 89, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 92, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 89, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 89, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 93, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 92, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 85, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 91, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 93, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 94, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 76, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 85, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 95, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 94, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 96, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 2, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 97, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 96, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 99, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 95, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 74, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 100, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 97, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 99, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 69, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 74, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 100, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 102, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_UNIFORM, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 103, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 102, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ABS, 104, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 105, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 103, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 104, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MAX, 107, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 105, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.125000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MIN, 106, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 107, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.750000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 111, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 112, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 111, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 114, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 112, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 112, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 115, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 112, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 112, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 116, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 115, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 108, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 114, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 116, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 120, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, -0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SAT, 121, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 120, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 123, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 121, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 121, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 124, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 121, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 121, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 125, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 3.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 124, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 117, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 123, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 125, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 126, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 108, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 117, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 127, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 126, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 128, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 4, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 129, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 128, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.500000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 131, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 127, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 106, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 132, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 129, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 131, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 101, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 106, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 132, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 133, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 5, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 5, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 134, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 133, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 5, 0xAA, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 135, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 134, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 5, 0xFF, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 136, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 135, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 37, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 137, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 136, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 69, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 138, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 137, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 69, 0x55, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 139, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 138, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 69, 0xAA, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 140, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 139, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 69, 0xFF, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 141, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 140, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 101, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 142, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 141, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.100000f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_SUB, 0, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 142, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MUL, 145, 0x1, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 0.333300f));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_ADD, 147, 0x8, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceConstant(Shader, 1.000000f));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 145, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 147, 0x3, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_ATTRIBUTE, 0, 0x4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 147, 0x4, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 0, 0x0, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_MOV, 144, 0xF, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddSourceIndexed(Shader, gcSL_TEMP, 147, 0xE4, gcSL_NOT_INDEXED, 0, gcSL_FLOAT));
    gcmONERROR(gcSHADER_AddOpcode(Shader, gcSL_NOP, 0, 0x0, gcSL_FLOAT));
    return gcvSTATUS_OK;

OnError:
    return status;
}

/******************************************************************************/

static const char _Egypt0Shader[] =
"#ifdef GL_ES"
    "precision mediump float;"
"#endif"
"uniform samplerCube uniSamplerCubeRad;"
"uniform samplerCube uniSamplerCubeRad2;"
"uniform float uniSamplerCubeDistance;"
"uniform float uniSamplerCubeDistance2;"
"uniform vec3 uniViewDir;"
"void main(){"
    "float w1=uniSamplerCubeDistance2/(uniSamplerCubeDistance+uniSamplerCubeDistance2);"
    "vec3 iriscolor=(textureCube(uniSamplerCubeRad,uniViewDir).rgb)*w1+(textureCube(uniSamplerCubeRad2,uniViewDir).rgb)*(1.-w1);"
    "float irisval=dot(iriscolor,vec3(0.3,0.59,0.11));"
    "irisval=sqrt(.67/irisval);"
    "gl_FragColor=vec4(floor(irisval)/3.0,fract(irisval),1.0,1.0);"
"}";

static gceSTATUS _Egypt0(gcSHADER Shader)
{
    gceSTATUS status;
    gcUNIFORM
        uniSamplerCubeRad,
        uniSamplerCubeRad2,
        uniSamplerCubeDistance,
        uniSamplerCubeDistance2,
        uniViewDir;

    /* uniform samplerCube uniSamplerCubeRad; */
    ADD_UNIFORM(uniSamplerCubeRad, SAMPLER_CUBIC);

    /* uniform samplerCube uniSamplerCubeRad2; */
    ADD_UNIFORM(uniSamplerCubeRad2, SAMPLER_CUBIC);

    /* uniform float uniSamplerCubeDistance; */
    ADD_UNIFORM(uniSamplerCubeDistance, FLOAT_X1);

    /* uniform float uniSamplerCubeDistance2; */
    ADD_UNIFORM(uniSamplerCubeDistance2, FLOAT_X1);

    /* uniform vec3 uniViewDir; */
    ADD_UNIFORM(uniViewDir, FLOAT_X3);

    /* void main() { */

    /* vec3 iris = textureCube(uniSamplerCubeRad, uniViewDir).rgb;
    **
    **      texld   t0.xyz, uniSamplerCubeRad, uniViewDir
    */
    OPCODE(TEXLD, 0, XYZ); UNIFORM(uniSamplerCubeRad, XYZW); UNIFORM(uniViewDir, XYZ);

    /* vec3 iris2 = (textureCube(uniSamplerCubeRad2, uniViewDir).rgb);
    **
    **      texld   t1.xyz, uniSamplerCubeRad2, uniViewDir
    */
    OPCODE(TEXLD, 1, XYZ); UNIFORM(uniSamplerCubeRad2, XYZW); UNIFORM(uniViewDir, XYZ);

    /* float w1 = uniSamplerCubeDistance2
    **          / (uniSamplerCubeDistance + uniSamplerCubeDistance2);
    **
    **      add     t2.x, uniSamplerCubeDistance, uniSamplerCubeDistance2
    **      rcp     t3.x, t2.x
    **      mul     t4.x, uniSamplerCubeDistance2, t3.x
    */
    OPCODE(ADD, 2, X); UNIFORM(uniSamplerCubeDistance, X); UNIFORM(uniSamplerCubeDistance2, X);
    OPCODE(RCP, 3, X); TEMP(2, X);
    OPCODE(MUL, 4, X); UNIFORM(uniSamplerCubeDistance2, X); TEMP(3, X);

    /* vec3 iriscolor = iris * w1 + iris2 * (1. - w1);
    **
    **      mul     t5.xyz, t0.xyz, t4.x
    **      sub     t6.x, 1, t4.x
    **      mul     t7.xyz, t1.xyz, t6.x
    **      add     t8.xyz, t5.xyz, t7.xyz
    */
    OPCODE(MUL, 5, XYZ); TEMP(0, XYZ); TEMP(4, X);
    OPCODE(SUB, 6, X); CONST(1.0f); TEMP(4, X);
    OPCODE(MUL, 7, XYZ); TEMP(1, XYZ); TEMP(6, X);
    OPCODE(ADD, 8, XYZ); TEMP(5, XYZ); TEMP(7, XYZ);

    /* float irisval = dot(iriscolor, vec3(0.3, 0.59, 0.11));
    **
    **      mov     t9.x, 0.3
    **      mov     t9.y, 0.59
    **      mov     t9.z, 0.11
    **      dp3     t10.x, t8.xyz, t9.xyz
    */
    OPCODE(MOV, 9, X); CONST(0.3f);
    OPCODE(MOV, 9, Y); CONST(0.59f);
    OPCODE(MOV, 9, Z); CONST(0.11f);
    OPCODE(DP3, 10, X); TEMP(8, XYZ); TEMP(9, XYZ);

    /* irisval = sqrt(.67 / irisval);
    **
    **      rcp     t11.x, t10.x
    **      mul     t12.x, .67, t11.x
    **      sqrt    t13.x, t12.x
    */
    OPCODE(RCP, 11, X); TEMP(10, X);
    OPCODE(MUL, 12, X); CONST(0.67f); TEMP(11, X);
    OPCODE(SQRT, 13, X); TEMP(12, X);

    /* gl_FragColor = vec4(floor(irisval) / 3.0, fract(irisval), 1.0, 1.0);
    **
    **      mov     t14.x, 1 / 3
    **      mov     t14.yzw, 1
    **      floor   t15.x, t13.x
    **      frac    t15.y, t13.x
    **      mul     t16.xy, t14.xy, t15.xy
    **      mov     t16.zw, t14.zw
    */
    OPCODE(MOV, 14, X); CONST(1.0f / 3.0f);
    OPCODE(MOV, 14, YZW); CONST(1.0f);
    OPCODE(FLOOR, 15, X); TEMP(13, X);
    OPCODE(FRAC, 15, Y); TEMP(13, X);
    OPCODE(MUL, 16, XY); TEMP(14, XY); TEMP(15, XY);
    OPCODE(MOV, 16, ZW); TEMP(14, ZW);
    COLOR(16);

    /* } */
    PACK();
    return gcvSTATUS_OK;

OnError:
    return status;
}

static const char _Egypt1Shader[] =
"#ifdef GL_ES"
    "precision highp float;"
"#endif"
"uniform highp mat4 uniModelViewProjectionMatrix;"
"uniform highp mat3 uniBonesM[32];"
"uniform highp vec3 uniBonesT[32];"
"attribute mediump vec4 atrVertex;"
"attribute highp vec4 atrBoneIndex;"
"attribute highp vec4 atrBoneWeight;"
"void main(void){"
    "ivec4 I=ivec4(atrBoneIndex);"
    "mat3 B3=uniBonesM[I.x]*atrBoneWeight.x+uniBonesM[I.y]*atrBoneWeight.y+uniBonesM[I.z]*atrBoneWeight.z+uniBonesM[I.w]*atrBoneWeight.w;"
    "vec3 T=uniBonesT[I.x]*atrBoneWeight.x+uniBonesT[I.y]*atrBoneWeight.y+uniBonesT[I.z]*atrBoneWeight.z+uniBonesT[I.w]*atrBoneWeight.w;"
    "vec3 boneVertex=B3*atrVertex.xyz+T;"
    "gl_Position=uniModelViewProjectionMatrix*vec4(boneVertex,1.0);"
"}";

static gceSTATUS _Egypt1(gcSHADER Shader)
{
    gceSTATUS status;
    gcUNIFORM uniModelViewProjectionMatrix, uniBonesM, uniBonesT;
    gcATTRIBUTE atrVertex, atrBoneIndex, atrBoneWeight;

    /* uniform highp mat4 uniModelViewProjectionMatrix; */
    ADD_UNIFORM(uniModelViewProjectionMatrix, FLOAT_4X4);

    /* uniform highp mat3 uniBonesM[32]; */
    ADD_UNIFORM_ARRAY(uniBonesM, FLOAT_3X3, 32);

    /* uniform highp vec3 uniBonesT[32]; */
    ADD_UNIFORM_ARRAY(uniBonesT, FLOAT_X3, 32);

    /* attribute mediump vec4 atrVertex; */
    ADD_ATTRIBUTE(atrVertex, FLOAT_X4);

    /* attribute highp vec4 atrBoneIndex; */
    ADD_ATTRIBUTE(atrBoneIndex, FLOAT_X4);

    /* attribute highp vec4 atrBoneWeight; */
    ADD_ATTRIBUTE(atrBoneWeight, FLOAT_X4);

    /* void main(void) { */

    /* ivec4 I = ivec4(atrBoneIndex);
    **
    **      mul     t0, atrBoneIndex, 3
    */
    OPCODE(MUL, 0, XYZW); ATTRIBUTE(atrBoneIndex, XYZW); CONST(3.0f);

    /* mat3 B3 = uniBonesM[I.x] * atrBoneWeight.x
    **         + uniBonesM[I.y] * atrBoneWeight.y
    **         + uniBonesM[I.z] * atrBoneWeight.z
    **         + uniBonesM[I.w] * atrBoneWeight.w;
    **
    **      mul     t1.xyz, uniBonesM[t0.x][0], atrBoneWeight.x
    **      mul     t2.xyz, uniBonesM[t0.x][1], atrBoneWeight.x
    **      mul     t3.xyz, uniBonesM[t0.x][2], atrBoneWeight.x
    **
    **      mul     t4.xyz, uniBonesM[t0.y][0], atrBoneWeight.y
    **      mul     t5.xyz, uniBonesM[t0.y][1], atrBoneWeight.y
    **      mul     t6.xyz, uniBonesM[t0.y][2], atrBoneWeight.y
    **      add     t7.xyz, t1.xyz, t4.xyz
    **      add     t8.xyz, t2.xyz, t5.xyz
    **      add     t9.xyz, t3.xyz, t6.xyz
    **
    **      mul     t10.xyz, uniBonesM[t0.z][0], atrBoneWeight.z
    **      mul     t11.xyz, uniBonesM[t0.z][1], atrBoneWeight.z
    **      mul     t12.xyz, uniBonesM[t0.z][2], atrBoneWeight.z
    **      add     t13.xyz, t7.xyz, t10.xyz
    **      add     t14.xyz, t8.xyz, t11.xyz
    **      add     t15.xyz, t9.xyz, t12.xyz
    **
    **      mul     t16.xyz, uniBonesM[t0.w][0], atrBoneWeight.w
    **      mul     t17.xyz, uniBonesM[t0.w][1], atrBoneWeight.w
    **      mul     t18.xyz, uniBonesM[t0.w][2], atrBoneWeight.w
    **      add     t19.xyz, t13.xyz, t16.xyz
    **      add     t20.xyz, t14.xyz, t17.xyz
    **      add     t21.xyz, t15.xyz, t18.xyz
    */
    OPCODE(MUL, 1, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 0, X); ATTRIBUTE(atrBoneWeight, X);
    OPCODE(MUL, 2, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 0, X); ATTRIBUTE(atrBoneWeight, X);
    OPCODE(MUL, 3, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 0, X); ATTRIBUTE(atrBoneWeight, X);

    OPCODE(MUL, 4, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 0, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(MUL, 5, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 0, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(MUL, 6, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 0, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(ADD, 7, XYZ); TEMP(1, XYZ); TEMP(4, XYZ);
    OPCODE(ADD, 8, XYZ); TEMP(2, XYZ); TEMP(5, XYZ);
    OPCODE(ADD, 9, XYZ); TEMP(3, XYZ); TEMP(6, XYZ);

    OPCODE(MUL, 10, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 0, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(MUL, 11, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 0, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(MUL, 12, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 0, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(ADD, 13, XYZ); TEMP(7, XYZ); TEMP(10, XYZ);
    OPCODE(ADD, 14, XYZ); TEMP(8, XYZ); TEMP(11, XYZ);
    OPCODE(ADD, 15, XYZ); TEMP(9, XYZ); TEMP(12, XYZ);

    OPCODE(MUL, 16, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 0, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(MUL, 17, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 0, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(MUL, 18, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 0, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(ADD, 19, XYZ); TEMP(13, XYZ); TEMP(16, XYZ);
    OPCODE(ADD, 20, XYZ); TEMP(14, XYZ); TEMP(17, XYZ);
    OPCODE(ADD, 21, XYZ); TEMP(15, XYZ); TEMP(18, XYZ);

    /* vec3 T = uniBonesT[I.x] * atrBoneWeight.x
    **        + uniBonesT[I.y] * atrBoneWeight.y
    **        + uniBonesT[I.z] * atrBoneWeight.z
    **        + uniBonesT[I.w] * atrBoneWeight.w;
    **
    **      mov     t22, atrBoneIndex
    **
    **      mul     t23.xyz, uniBonesT[t22.x], atrBoneWeigth.x
    **      mul     t24.xyz, uniBonesT[t22.y], atrBoneWeigth.y
    **      mul     t25.xyz, uniBonesT[t22.z], atrBoneWeigth.z
    **      mul     t26.xyz, uniBonesT[t22.w], atrBoneWeigth.w
    **      add     t27.xyz, t23.xyz, t24.xyz
    **      add     t28.xyz, t27.xyz, t25.xyz
    **      add     t29.xyz, t28.xyz, t26.xyz
    */
    OPCODE(MOV, 22, XYZW); ATTRIBUTE(atrBoneIndex, XYZW);

    OPCODE(MUL, 23, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 22, X); ATTRIBUTE(atrBoneWeight, X);
    OPCODE(MUL, 24, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 22, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(MUL, 25, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 22, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(MUL, 26, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 22, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(ADD, 27, XYZ); TEMP(23, XYZ); TEMP(24, XYZ);
    OPCODE(ADD, 28, XYZ); TEMP(27, XYZ); TEMP(25, XYZ);
    OPCODE(ADD, 29, XYZ); TEMP(28, XYZ); TEMP(26, XYZ);

    /* vec3 boneVertex = T + B3 * atrVertex.xyz;
    **
    **      mul     t30.xyz, t19.xyz, atrVertex.x
    **      mul     t31.xyz, t20.xyz, atrVertex.y
    **      mul     t32.xyz, t21.xyz, atrVertex.z
    **      add     t33.xyz, t29.xyz, t30.xyz
    **      add     t34.xyz, t33.xyz, t31.xyz
    **      add     t35.xyz, t34.xyz, t32.xyz
    */
    OPCODE(MUL, 30, XYZ); TEMP(19, XYZ); ATTRIBUTE(atrVertex, X);
    OPCODE(MUL, 31, XYZ); TEMP(20, XYZ); ATTRIBUTE(atrVertex, Y);
    OPCODE(MUL, 32, XYZ); TEMP(21, XYZ); ATTRIBUTE(atrVertex, Z);
    OPCODE(ADD, 33, XYZ); TEMP(29, XYZ); TEMP(30, XYZ);
    OPCODE(ADD, 34, XYZ); TEMP(33, XYZ); TEMP(31, XYZ);
    OPCODE(ADD, 35, XYZ); TEMP(34, XYZ); TEMP(32, XYZ);

    /* gl_Position = uniModelViewProjectionMatrix * vec4(boneVertex, 1.0);
    **
    **      mul     t36, uniModelViewProjectionMatrix[0], t35.x
    **      mul     t37, uniModelViewProjectionMatrix[1], t35.y
    **      mul     t38, uniModelViewProjectionMatrix[2], t35.z
    **      add     t39, t36, t37
    **      add     t40, t39, t38
    **      add     t41, t40, uniModelViewProjectionMatrix[3]
    */
    OPCODE(MUL, 36, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 0); TEMP(35, X);
    OPCODE(MUL, 37, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 1); TEMP(35, Y);
    OPCODE(MUL, 38, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 2); TEMP(35, Z);
    OPCODE(ADD, 39, XYZW); TEMP(36, XYZW); TEMP(37, XYZW);
    OPCODE(ADD, 40, XYZW); TEMP(39, XYZW); TEMP(38, XYZW);
    OPCODE(ADD, 41, XYZW); TEMP(40, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 3);
    POSITION(41);

    /* } */
    PACK();
    return gcvSTATUS_OK;

OnError:
    return status;
}

static const char _Egypt2Shader[] =
"#ifdef GL_ES"
    "precision highp float;"
"#endif"
"uniform highp mat4 uniModelViewProjectionMatrix;"
"uniform highp mat3 uniBonesM[32];"
"uniform highp vec3 uniBonesT[32];"
"uniform highp mat4 uniWorldModelMatrix;"
"uniform highp mat3 uniWorldNormalMatrix;"
"uniform highp vec4 uniWorldCameraPos;"
"uniform mediump mat4 uniSunShadowMatrix;"
"attribute mediump vec4 atrVertex;"
"attribute highp vec4 atrBoneIndex;"
"attribute highp vec4 atrBoneWeight;"
"attribute mediump vec3 atrNormal;"
"attribute mediump vec2 atrTexcoordColor;"
"attribute mediump vec2 atrTexcoordBump;"
"attribute mediump vec3 atrTangent;"
"varying mediump vec2 varTexcoordColor;"
"varying mediump vec2 varTexcoordBump;"
"varying mediump vec3 varWorldNormal;"
"varying lowp vec3 varWorldTangent;"
"varying lowp vec3 varWorldBiTangent;"
"varying mediump vec3 varWorldEyeDir;"
"varying mediump vec2 varVertex2;"
"void main(void){"
    "varTexcoordColor=atrTexcoordColor;"
    "varTexcoordBump=atrTexcoordBump;"
    "ivec4 I=ivec4(atrBoneIndex);"
    "mat3 B3=uniBonesM[I.x]*atrBoneWeight.x+uniBonesM[I.y]*atrBoneWeight.y+uniBonesM[I.z]*atrBoneWeight.z+uniBonesM[I.w]*atrBoneWeight.w;"
    "vec3 T=uniBonesT[I.x]*atrBoneWeight.x+uniBonesT[I.y]*atrBoneWeight.y+uniBonesT[I.z]*atrBoneWeight.z+uniBonesT[I.w]*atrBoneWeight.w;"
    "vec3 boneVertex=B3*atrVertex.xyz+T;"
    "gl_Position=uniModelViewProjectionMatrix*vec4(boneVertex,1.0);"
    "varWorldNormal=normalize(uniWorldNormalMatrix*(B3*atrNormal));"
    "varWorldTangent=normalize(uniWorldNormalMatrix*(B3*atrTangent)).xyz;"
    "varWorldBiTangent=cross(varWorldNormal,varWorldTangent);"
    "vec4 worldVertex=uniWorldModelMatrix*atrVertex;"
    "varWorldEyeDir=normalize((uniWorldModelMatrix*vec4(boneVertex,1.0)).xyz-uniWorldCameraPos.xyz);"
    "varVertex2=vec2(uniSunShadowMatrix*vec4(boneVertex,1.0)).xy;"
"}";

static gceSTATUS _Egypt2(gcSHADER Shader)
{
    gceSTATUS status;
    gcUNIFORM
           uniModelViewProjectionMatrix,
           uniBonesM,
           uniBonesT,
           uniWorldModelMatrix,
           uniWorldNormalMatrix,
           uniWorldCameraPos,
           uniSunShadowMatrix;
    gcATTRIBUTE
            atrVertex,
            atrBoneIndex,
            atrBoneWeight,
            atrNormal,
            atrTexcoordColor,
            atrTexcoordBump,
            atrTangent;

    /* uniform highp mat4 uniModelViewProjectionMatrix; */
    ADD_UNIFORM(uniModelViewProjectionMatrix, FLOAT_4X4);

    /* uniform highp mat3 uniBonesM[32]; */
    ADD_UNIFORM_ARRAY(uniBonesM, FLOAT_3X3, 32);

    /* uniform highp vec3 uniBonesT[32]; */
    ADD_UNIFORM_ARRAY(uniBonesT, FLOAT_X3, 32);

    /* uniform highp mat4 uniWorldModelMatrix; */
    ADD_UNIFORM(uniWorldModelMatrix, FLOAT_4X4);

    /* uniform highp mat3 uniWorldNormalMatrix; */
    ADD_UNIFORM(uniWorldNormalMatrix, FLOAT_3X3);

    /* uniform highp vec4 uniWorldCameraPos; */
    ADD_UNIFORM(uniWorldCameraPos, FLOAT_X4);

    /* uniform mediump mat4 uniSunShadowMatrix; */
    ADD_UNIFORM(uniSunShadowMatrix, FLOAT_4X4);

    /* attribute mediump vec4 atrVertex; */
    ADD_ATTRIBUTE(atrVertex, FLOAT_X4);

    /* attribute highp vec4 atrBoneIndex; */
    ADD_ATTRIBUTE(atrBoneIndex, FLOAT_X4);

    /* attribute highp vec4 atrBoneWeight; */
    ADD_ATTRIBUTE(atrBoneWeight, FLOAT_X4);

    /* attribute mediump vec3 atrNormal; */
    ADD_ATTRIBUTE(atrNormal, FLOAT_X3);

    /* attribute mediump vec2 atrTexcoordColor; */
    ADD_ATTRIBUTE(atrTexcoordColor, FLOAT_X2);

    /* attribute mediump vec2 atrTexcoordBump; */
    ADD_ATTRIBUTE(atrTexcoordBump, FLOAT_X2);

    /* attribute mediump vec3 atrTangent; */
    ADD_ATTRIBUTE(atrTangent, FLOAT_X3);

    /* varying mediump vec2 varTexcoordColor; */
    ADD_OUTPUT(varTexcoordColor, FLOAT_X2);

    /* varying mediump vec2 varTexcoordBump; */
    ADD_OUTPUT(varTexcoordBump, FLOAT_X2);

    /* varying mediump vec3 varWorldNormal; */
    ADD_OUTPUT(varWorldNormal, FLOAT_X3);

    /* varying lowp vec3 varWorldTangent; */
    ADD_OUTPUT(varWorldTangent, FLOAT_X3);

    /* varying lowp vec3 varWorldBiTangent; */
    ADD_OUTPUT(varWorldBiTangent, FLOAT_X3);

    /* varying mediump vec3 varWorldEyeDir; */
    ADD_OUTPUT(varWorldEyeDir, FLOAT_X3);

    /* varying mediump vec2 varVertex2; */
    ADD_OUTPUT(varVertex2, FLOAT_X2);

    /* void main(void){ */

    /* varTexcoordColor = atrTexcoordColor;
    **
    **      mov     t0.xy, atrTexcoordColor.xy
    */
    OPCODE(MOV, 0, XY); ATTRIBUTE(atrTexcoordColor, XY);
    OUTPUT(varTexcoordColor, 0);

    /* varTexcoordBump = atrTexcoordBump;
    **
    **      mov     t1.xy, atrTexcoordBump.xy
    */
    OPCODE(MOV, 1, XY); ATTRIBUTE(atrTexcoordBump, XY);
    OUTPUT(varTexcoordBump, 1);

    /* ivec4 I = ivec4(atrBoneIndex);
    **
    **      mul     t2, atrBoneIndex, 3
    */
    OPCODE(MUL, 2, XYZW); ATTRIBUTE(atrBoneIndex, XYZW); CONST(3.0f);

    /* mat3 B3 = uniBonesM[I.x] * atrBoneWeight.x
    **         + uniBonesM[I.y] * atrBoneWeight.y
    **         + uniBonesM[I.z] * atrBoneWeight.z
    **         + uniBonesM[I.w] * atrBoneWeight.w;
    **
    **      mul     t3.xyz, uniBonesM[t2.x][0], atrBoneWeight.x
    **      mul     t4.xyz, uniBonesM[t2.x][1], atrBoneWeight.x
    **      mul     t5.xyz, uniBonesM[t2.x][2], atrBoneWeight.x
    **      mul     t6.xyz, uniBonesM[t2.y][0], atrBoneWeight.y
    **      mul     t7.xyz, uniBonesM[t2.y][1], atrBoneWeight.y
    **      mul     t8.xyz, uniBonesM[t2.y][2], atrBoneWeight.y
    **      mul     t9.xyz, uniBonesM[t2.z][0], atrBoneWeight.z
    **      mul     t10.xyz, uniBonesM[t2.z][1], atrBoneWeight.z
    **      mul     t11.xyz, uniBonesM[t2.z][2], atrBoneWeight.z
    **      mul     t12.xyz, uniBonesM[t2.w][0], atrBoneWeight.w
    **      mul     t13.xyz, uniBonesM[t2.w][1], atrBoneWeight.w
    **      mul     t14.xyz, uniBonesM[t2.w][2], atrBoneWeight.w
    **      add     t15.xyz, t3.xyz, t6.xyz
    **      add     t16.xyz, t4.xyz, t7.xyz
    **      add     t17.xyz, t5.xyz, t8.xyz
    **      add     t18.xyz, t15.xyz, t9.xyz
    **      add     t19.xyz, t16.xyz, t10.xyz
    **      add     t20.xyz, t17.xyz, t11.xyz
    **      add     t21.xyz, t18.xyz, t12.xyz
    **      add     t22.xyz, t19.xyz, t13.xyz
    **      add     t23.xyz, t20.xyz, t14.xyz
    */
    OPCODE(MUL,  3, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 2, X); ATTRIBUTE(atrBoneWeight, X);
    OPCODE(MUL,  4, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 2, X); ATTRIBUTE(atrBoneWeight, X);
    OPCODE(MUL,  5, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 2, X); ATTRIBUTE(atrBoneWeight, X);
    OPCODE(MUL,  6, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 2, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(MUL,  7, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 2, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(MUL,  8, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 2, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(MUL,  9, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 2, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(MUL, 10, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 2, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(MUL, 11, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 2, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(MUL, 12, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 0, 2, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(MUL, 13, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 1, 2, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(MUL, 14, XYZ); UNIFORM_ARRAY_INDEX(uniBonesM, XYZ, 2, 2, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(ADD, 15, XYZ); TEMP( 3, XYZ); TEMP( 6, XYZ);
    OPCODE(ADD, 16, XYZ); TEMP( 4, XYZ); TEMP( 7, XYZ);
    OPCODE(ADD, 17, XYZ); TEMP( 5, XYZ); TEMP( 8, XYZ);
    OPCODE(ADD, 18, XYZ); TEMP(15, XYZ); TEMP( 9, XYZ);
    OPCODE(ADD, 19, XYZ); TEMP(16, XYZ); TEMP(10, XYZ);
    OPCODE(ADD, 20, XYZ); TEMP(17, XYZ); TEMP(11, XYZ);
    OPCODE(ADD, 21, XYZ); TEMP(18, XYZ); TEMP(12, XYZ);
    OPCODE(ADD, 22, XYZ); TEMP(19, XYZ); TEMP(13, XYZ);
    OPCODE(ADD, 23, XYZ); TEMP(20, XYZ); TEMP(14, XYZ);

    /* vec3 T = uniBonesT[I.x] * atrBoneWeight.x
    **        + uniBonesT[I.y] * atrBoneWeight.y
    **        + uniBonesT[I.z] * atrBoneWeight.z
    **        + uniBonesT[I.w] * atrBoneWeight.w;
    **
    **      mov     t24, atrBoneIndex
    **      mul     t25.xyz, uniBonesT[t24.x], atrBoneWeight.x
    **      mul     t26.xyz, uniBonesT[t24.y], atrBoneWeight.y
    **      mul     t27.xyz, uniBonesT[t24.z], atrBoneWeight.z
    **      mul     t28.xyz, uniBonesT[t24.w], atrBoneWeight.w
    **      add     t29.xyz, t25.xyz, t26.xyz
    **      add     t30.xyz, t29.xyz, t27.xyz
    **      add     t31.xyz, t30.xyz, t28.xyz
    */
    OPCODE(MOV, 24, XYZW); ATTRIBUTE(atrBoneIndex, XYZW);
    OPCODE(MUL, 25, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 24, X); ATTRIBUTE(atrBoneWeight, X);
    OPCODE(MUL, 26, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 24, Y); ATTRIBUTE(atrBoneWeight, Y);
    OPCODE(MUL, 27, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 24, Z); ATTRIBUTE(atrBoneWeight, Z);
    OPCODE(MUL, 28, XYZ); UNIFORM_ARRAY_INDEX(uniBonesT, XYZ, 0, 24, W); ATTRIBUTE(atrBoneWeight, W);
    OPCODE(ADD, 29, XYZ); TEMP(25, XYZ); TEMP(26, XYZ);
    OPCODE(ADD, 30, XYZ); TEMP(29, XYZ); TEMP(27, XYZ);
    OPCODE(ADD, 31, XYZ); TEMP(30, XYZ); TEMP(28, XYZ);

    /* vec3 boneVertex = T + B3 * atrVertex.xyz;
    **
    **      mul     t32.xyz, t21.xyz, atrVertex.x
    **      mul     t33.xyz, t22.xyz, atrVertex.y
    **      mul     t34.xyz, t23.xyz, atrVertex.z
    **      add     t35.xyz, t31.xyz, t32.xyz
    **      add     t36.xyz, t35.xyz, t33.xyz
    **      add     t37.xyz, t36.xyz, t34.xyz
    */
    OPCODE(MUL, 32, XYZ); TEMP(21, XYZ); ATTRIBUTE(atrVertex, X);
    OPCODE(MUL, 33, XYZ); TEMP(22, XYZ); ATTRIBUTE(atrVertex, Y);
    OPCODE(MUL, 34, XYZ); TEMP(23, XYZ); ATTRIBUTE(atrVertex, Z);
    OPCODE(ADD, 35, XYZ); TEMP(31, XYZ); TEMP(32, XYZ);
    OPCODE(ADD, 36, XYZ); TEMP(35, XYZ); TEMP(33, XYZ);
    OPCODE(ADD, 37, XYZ); TEMP(36, XYZ); TEMP(34, XYZ);

    /* gl_Position = uniModelViewProjectionMatrix * vec4(boneVertex, 1.0);
    **
    **      mul     t38, uniModelViewProjectionMatrix[0], t37.x
    **      mul     t39, uniModelViewProjectionMatrix[1], t37.y
    **      mul     t40, uniModelViewProjectionMatrix[2], t37.z
    **      add     t41, t38, t39
    **      add     t42, t41, t40
    **      add     t43, t42, uniModelViewProjectionMatrix[3]
    */
    OPCODE(MUL, 38, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 0); TEMP(37, X);
    OPCODE(MUL, 39, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 1); TEMP(37, Y);
    OPCODE(MUL, 40, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 2); TEMP(37, Z);
    OPCODE(ADD, 41, XYZW); TEMP(38, XYZW); TEMP(39, XYZW);
    OPCODE(ADD, 42, XYZW); TEMP(41, XYZW); TEMP(40, XYZW);
    OPCODE(ADD, 43, XYZW); TEMP(42, XYZW); UNIFORM_ARRAY(uniModelViewProjectionMatrix, XYZW, 3);
    POSITION(43);

    /* varWorldNormal = normalize(uniWorldNormalMatrix * (B3 * atrNormal));
    **
    **      mul     t44.xyz, t21.xyz, atrNormal.x
    **      mul     t45.xyz, t22.xyz, atrNormal.y
    **      mul     t46.xyz, t23.xyz, atrNormal.z
    **      add     t47.xyz, t44.xyz, t45.xyz
    **      add     t48.xyz, t47.xyz, t46.xyz
    **      mul     t49.xyz, uniWorldNormalMatrix[0], t48.x
    **      mul     t50.xyz, uniWorldNormalMatrix[1], t48.y
    **      mul     t51.xyz, uniWorldNormalMatrix[2], t48.z
    **      add     t52.xyz, t49.xyz, t50.xyz
    **      add     t53.xyz, t52.xyz, t51.xyz
    **      norm    t54.xyz, t53.xyz
    */
    OPCODE(MUL, 44, XYZ); TEMP(21, XYZ); ATTRIBUTE(atrNormal, X);
    OPCODE(MUL, 45, XYZ); TEMP(22, XYZ); ATTRIBUTE(atrNormal, Y);
    OPCODE(MUL, 46, XYZ); TEMP(23, XYZ); ATTRIBUTE(atrNormal, Z);
    OPCODE(ADD, 47, XYZ); TEMP(44, XYZ); TEMP(45, XYZ);
    OPCODE(ADD, 48, XYZ); TEMP(47, XYZ); TEMP(46, XYZ);
    OPCODE(MUL, 49, XYZ); UNIFORM_ARRAY(uniWorldNormalMatrix, XYZ, 0); TEMP(48, X);
    OPCODE(MUL, 50, XYZ); UNIFORM_ARRAY(uniWorldNormalMatrix, XYZ, 1); TEMP(48, Y);
    OPCODE(MUL, 51, XYZ); UNIFORM_ARRAY(uniWorldNormalMatrix, XYZ, 2); TEMP(48, Z);
    OPCODE(ADD, 52, XYZ); TEMP(49, XYZ); TEMP(50, XYZ);
    OPCODE(ADD, 53, XYZ); TEMP(52, XYZ); TEMP(51, XYZ);
    OPCODE(NORM, 54, XYZ); TEMP(53, XYZ);
    OUTPUT(varWorldNormal, 54);

    /* varWorldTangent = normalize(uniWorldNormalMatrix * (B3 * atrTangent)).xyz;
    **
    **      mul     t55.xyz, t21.xyz, atrTangent.x
    **      mul     t56.xyz, t22.xyz, atrTangent.y
    **      mul     t57.xyz, t23.xyz, atrTangent.z
    **      add     t58.xyz, t55.xyz, t56.xyz
    **      add     t59.xyz, t58.xyz, t57.xyz
    **      mul     t60.xyz, uniWorldNormalMatrix[0], t59.x
    **      mul     t61.xyz, uniWorldNormalMatrix[1], t59.y
    **      mul     t62.xyz, uniWorldNormalMatrix[2], t59.z
    **      add     t63.xyz, t60.xyz, t61.xyz
    **      add     t64.xyz, t63.xyz, t62.xyz
    **      norm    t65.xyz, t64.xyz
    */
    OPCODE(MUL, 55, XYZ); TEMP(21, XYZ); ATTRIBUTE(atrTangent, X);
    OPCODE(MUL, 56, XYZ); TEMP(22, XYZ); ATTRIBUTE(atrTangent, Y);
    OPCODE(MUL, 57, XYZ); TEMP(23, XYZ); ATTRIBUTE(atrTangent, Z);
    OPCODE(ADD, 58, XYZ); TEMP(55, XYZ); TEMP(56, XYZ);
    OPCODE(ADD, 59, XYZ); TEMP(58, XYZ); TEMP(57, XYZ);
    OPCODE(MUL, 60, XYZ); UNIFORM_ARRAY(uniWorldNormalMatrix, XYZ, 0); TEMP(59, X);
    OPCODE(MUL, 61, XYZ); UNIFORM_ARRAY(uniWorldNormalMatrix, XYZ, 1); TEMP(59, Y);
    OPCODE(MUL, 62, XYZ); UNIFORM_ARRAY(uniWorldNormalMatrix, XYZ, 2); TEMP(59, Z);
    OPCODE(ADD, 63, XYZ); TEMP(60, XYZ); TEMP(61, XYZ);
    OPCODE(ADD, 64, XYZ); TEMP(63, XYZ); TEMP(62, XYZ);
    OPCODE(NORM, 65, XYZ); TEMP(64, XYZ);
    OUTPUT(varWorldTangent, 65);

    /* varWorldBiTangent = cross(varWorldNormal, varWorldTangent);
    **
    **      cross   t66.xyz, t54.xyz, t65.xyz
    */
    OPCODE(CROSS, 66, XYZ); TEMP(54, XYZ); TEMP(65, XYZ);
    OUTPUT(varWorldBiTangent, 66);

    /* varWorldEyeDir = normalize((uniWorldModelMatrix * vec4(boneVertex, 1.0)).xyz - uniWorldCameraPos.xyz);
    **
    **      mul     t67.xyz, uniWorldModelMatrix[0].xyz, t37.x
    **      mul     t68.xyz, uniWorldModelMatrix[1].xyz, t37.y
    **      mul     t69.xyz, uniWorldModelMatrix[2].xyz, t37.z
    **      add     t70.xyz, t67.xyz, t68.xyz
    **      add     t71.xyz, t70.xyz, t69.xyz
    **      add     t72.xyz, t71.xyz, uniWorldModelMatrix[3].xyz
    **      sub     t73.xyz, y72.xyz, uniWorldCameraPos.xyz
    **      norm    t74.xyz, t73.xyz
    */
    OPCODE(MUL, 67, XYZ); UNIFORM_ARRAY(uniWorldModelMatrix, XYZ, 0); TEMP(37, X);
    OPCODE(MUL, 68, XYZ); UNIFORM_ARRAY(uniWorldModelMatrix, XYZ, 1); TEMP(37, Y);
    OPCODE(MUL, 69, XYZ); UNIFORM_ARRAY(uniWorldModelMatrix, XYZ, 2); TEMP(37, Z);
    OPCODE(ADD, 70, XYZ); TEMP(67, XYZ); TEMP(68, XYZ);
    OPCODE(ADD, 71, XYZ); TEMP(70, XYZ); TEMP(69, XYZ);
    OPCODE(ADD, 72, XYZ); TEMP(71, XYZ); UNIFORM_ARRAY(uniWorldModelMatrix, XYZ, 3);
    OPCODE(SUB, 73, XYZ); TEMP(72, XYZ); UNIFORM(uniWorldCameraPos, XYZ);
    OPCODE(NORM, 74, XYZ); TEMP(73, XYZ);
    OUTPUT(varWorldEyeDir, 74);

    /* varVertex2 = vec2(uniSunShadowMatrix * vec4(boneVertex, 1.0)).xy;
    **
    **      mul     t75.xy, uniSunShadowMatrix[0], t37.x
    **      mul     t76.xy, uniSunShadowMatrix[1], t37.y
    **      mul     t77.xy, uniSunShadowMatrix[2], t37.z
    **      add     t78.xy, t75.xy, t76.xy
    **      add     t79.xy, t78.xy, t77.xy
    **      add     t80.xy, t79.xy, uniSunShadowMatrix[3]
    */
    OPCODE(MUL, 75, XY); UNIFORM_ARRAY(uniSunShadowMatrix, XY, 0); TEMP(37, X);
    OPCODE(MUL, 76, XY); UNIFORM_ARRAY(uniSunShadowMatrix, XY, 1); TEMP(37, Y);
    OPCODE(MUL, 77, XY); UNIFORM_ARRAY(uniSunShadowMatrix, XY, 2); TEMP(37, Z);
    OPCODE(ADD, 78, XY); TEMP(75, XY); TEMP(76, XY);
    OPCODE(ADD, 79, XY); TEMP(78, XY); TEMP(77, XY);
    OPCODE(ADD, 80, XY); TEMP(79, XY); UNIFORM_ARRAY(uniSunShadowMatrix, XY, 3);
    OUTPUT(varVertex2, 80);

    /* } */
    PACK();
    return gcvSTATUS_OK;

OnError:
    return status;
}

#if EGYPT3
static const char _Egypt3Shader[] =
"#ifdef GL_ES"
    "precision mediump float;"
"#endif"
"uniform mediump sampler2D uniSampler2DColor;"
"uniform lowp sampler2D uniSampler2DShadow;"
"uniform lowp float uniDisableShadow[3];"
"uniform lowp sampler2D uniSampler2DLight;"
"uniform lowp float uniSampler2DIris;"
"varying mediump vec2 varTexcoordColor;"
"varying mediump vec2 varTexcoordLight;"
"varying mediump vec4 varTexcoordShadow[3];"
"void main(void){"
    "mediump vec4 v4color=texture2D(uniSampler2DColor,varTexcoordColor);"
    "v4color*=texture2D(uniSampler2DLight,varTexcoordLight);"
    "if(uniDisableShadow[0]==0.0){"
        "vec2 min0=min(abs(varTexcoordShadow[0].xy),vec2(varTexcoordShadow[0].w));"
        "if(varTexcoordShadow[0].xy==min0)"
            "v4color*=texture2DProj(uniSampler2DShadow,varTexcoordShadow[0]).x;"
    "}"
    "if(uniDisableShadow[1]==0.0){"
        "vec2 min1=min(abs(varTexcoordShadow[1].xy),vec2(varTexcoordShadow[1].w));"
        "if(varTexcoordShadow[1].xy==min1)"
            "v4color*=texture2DProj(uniSampler2DShadow,varTexcoordShadow[1]).y;"
    "}"
    "if(uniDisableShadow[2]==0.0){"
        "vec2 min2=min(abs(varTexcoordShadow[2].xy),vec2(varTexcoordShadow[2].w));"
        "if(varTexcoordShadow[2].xy==min2)"
            "v4color*=texture2DProj(uniSampler2DShadow,varTexcoordShadow[2]).z;"
    "}"
    "v4color*=uniSampler2DIris;"
    "gl_FragColor=vec4(v4color.xyz,1.0);"
"}";

static gceSTATUS _Egypt3(gcSHADER Shader)
{
    gceSTATUS status;
    gcUNIFORM
        uniSampler2DColor,
        uniSampler2DShadow,
        uniDisableShadow,
        uniSampler2DLight,
        uniSampler2DIris;
    gcATTRIBUTE
        varTexcoordColor,
        varTexcoordLight,
        varTexcoordShadow;

    /* uniform mediump sampler2D uniSampler2DColor; */
    ADD_UNIFORM(uniSampler2DColor, SAMPLER_2D);

    /* uniform lowp sampler2D uniSampler2DShadow; */
    ADD_UNIFORM(uniSampler2DShadow, SAMPLER_2D);

    /* uniform lowp float uniDisableShadow[3]; */
    ADD_UNIFORM_ARRAY(uniDisableShadow, FLOAT_X1, 3);

    /* uniform lowp sampler2D uniSampler2DLight; */
    ADD_UNIFORM(uniSampler2DLight, SAMPLER_2D);

    /* uniform lowp float uniSampler2DIris; */
    ADD_UNIFORM(uniSampler2DIris, FLOAT_X1);

    /* varying mediump vec2 varTexcoordColor; */
    ADD_ATTRIBUTE_TX(varTexcoordColor, FLOAT_X2);

    /* varying mediump vec2 varTexcoordLight; */
    ADD_ATTRIBUTE_TX(varTexcoordLight, FLOAT_X2);

    /* varying mediump vec4 varTexcoordShadow[3]; */
    ADD_ATTRIBUTE_ARRAY_TX(varTexcoordShadow, FLOAT_X4, 3);

    /* void main(void) { */

    /* mediump vec3 v4color = texture2D(uniSampler2DColor, varTexcoordColor);
    **
    **      texld   t0.xyz, uniSampler2DColor, varTexcoordColor.xy
    */
    OPCODE(TEXLD, 0, XYZ); UNIFORM(uniSampler2DColor, XYZW); ATTRIBUTE(varTexcoordColor, XY);

    /* vec3 light = texture2D(uniSampler2DLight,varTexcoordLight);
    **
    **      texld   t1.xyz, uniSampler2DLight, varTexcoordLight.xy
    */
    OPCODE(TEXLD, 1, XYZ); UNIFORM(uniSampler2DLight, XYZW); ATTRIBUTE(varTexcoordLight, XY);

    /* vec3 shadow = vec3(1.0);
    **
    **      mov     t2.xyz, 1
    */
    OPCODE(MOV, 2, XYZ); CONST(1.0f);

    /* if (uniDisableShadow[0] == 0.0) {
    **
    **      jmp.nz  1, uniDisableShadow[0].x
    */
    OPCODE_COND(JMP, NOT_ZERO, 1); UNIFORM_ARRAY(uniDisableShadow, X, 0);

    /* vec2 min0 = min(abs(varTexcoordShadow[0].xy), vec2(varTexcoordShadow[0].w));
    **
    **      abs     t3.xy, varTexcoordShadow[0].xy
    **      min     t4.xy, t3.xy, varTexcoordShadow[0].w
    */
    OPCODE(ABS, 3, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 0);
    OPCODE(MIN, 4, XY); TEMP(3, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, W, 0);

    /* if (varTexcoordShadow[0].xy == min0)
    **
    **      jmp.ne  1, varTexcoordShadow[0].xy, t4.xy
    */
    OPCODE_COND(JMP, NOT_EQUAL, 1); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 0); TEMP(4, XY);

    /* shadow.x = texture2DProj(uniSampler2DShadow, varTexcoordShadow[0]).x;
    **
    **      texldp  t2.x, uniSampler2DShadow, varTexcoordShadow[0]
    */
    OPCODE(TEXLDP, 2, X); UNIFORM(uniSampler2DShadow, XYZW); ATTRIBUTE_ARRAY(varTexcoordShadow, XYZW, 0);

    LABEL(1);
    /* if (uniDisableShadow[1] == 0.0) {
    **
    **      jmp.nz  2, uniDisableShadow[1].x
    */
    OPCODE_COND(JMP, NOT_ZERO, 2); UNIFORM_ARRAY(uniDisableShadow, X, 1);

    /* vec2 min1 = min(abs(varTexcoordShadow[1].xy), vec2(varTexcoordShadow[1].w));
    **
    **      abs     t5.xy, varTexcoordShadow[1].xy
    **      min     t6.xy, t5.xy, varTexcoordShadow[1].w
    */
    OPCODE(ABS, 5, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 1);
    OPCODE(MIN, 6, XY); TEMP(5, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, W, 1);

    /* if (varTexcoordShadow[1].xy == min1)
    **
    **      jmp.ne  2, varTexcoordShadow[1].xy, t6.xy
    */
    OPCODE_COND(JMP, NOT_EQUAL, 2); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 1); TEMP(6, XY);

    /* shadow.y = texture2DProj(uniSampler2DShadow, varTexcoordShadow[1]).y;
    **
    **      texldp  t2.y, uniSampler2DShadow, varTexcoordShadow[1]
    */
    OPCODE(TEXLDP, 2, Y); UNIFORM(uniSampler2DShadow, XYZW); ATTRIBUTE_ARRAY(varTexcoordShadow, XYZW, 1);

    LABEL(2);
    /* if (uniDisableShadow[2] == 0.0) {
    **
    **      jmp.nz  3, uniDisableShadow[2].x
    */
    OPCODE_COND(JMP, NOT_ZERO, 3); UNIFORM_ARRAY(uniDisableShadow, X, 2);

    /* vec2 min2 = min(abs(varTexcoordShadow[2].xy), vec2(varTexcoordShadow[2].w));
    **
    **      abs     t7.xy, varTexcoordShadow[2].xy
    **      min     t8.xy, t7.xy, varTexcoordShadow[2].w
    */
    OPCODE(ABS, 7, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 2);
    OPCODE(MIN, 8, XY); TEMP(7, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, W, 2);

    /* if (varTexcoordShadow[2].xy == min2)
    **
    **      jmp.ne  3, varTexcoordShadow[2].xy, t8.xy
    */
    OPCODE_COND(JMP, NOT_EQUAL, 3); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 2); TEMP(8, XY);

    /* shadow.x = texture2DProj(uniSampler2DShadow, varTexcoordShadow[2]).x;
    **
    **      texldp  t2.z, uniSampler2DShadow, varTexcoordShadow[2]
    */
    OPCODE(TEXLDP, 2, Z); UNIFORM(uniSampler2DShadow, XYZW); ATTRIBUTE_ARRAY(varTexcoordShadow, XYZW, 2);

    LABEL(3);
    /* v4color *= light * shadow.x * shadow.y * shadow.z * uniSampler2DIris;
    **
    **      mul     t9.xyz, t0.xyz, t1.xyz
    **      mul     t10.xyz, t9.xyz, t2.x
    **      mul     t11.xyz, t10.xyz, t2.y
    **      mul     t12.xyz, t11.xyz, t2.z
    **      mul     t13.xyz, t12.xyz, uniSampler2DIris.x
    */
    OPCODE(MUL,  9, XYZ); TEMP( 0, XYZ); TEMP(1, XYZ);
    OPCODE(MUL, 10, XYZ); TEMP( 9, XYZ); TEMP(2, X);
    OPCODE(MUL, 11, XYZ); TEMP(10, XYZ); TEMP(2, Y);
    OPCODE(MUL, 12, XYZ); TEMP(11, XYZ); TEMP(2, Z);
    OPCODE(MUL, 13, XYZ); TEMP(12, XYZ); UNIFORM(uniSampler2DIris, X);

    /* gl_FragColor = vec4(v4color.xyz, 1.0);
    **
    **      mov     t14.xyz, t13.xyz
    **      mov     t14.w, 1
    */
    OPCODE(MOV, 14, XYZ); TEMP(13, XYZ);
    OPCODE(MOV, 14, W); CONST(1.0f);
    COLOR(14);

    /* } */
    PACK();
    return gcvSTATUS_OK;

OnError:
    return status;
}
#endif

#if EGYPT4
static const char _Egypt4Shader[] =
"#ifdef GL_ES"
"precision mediump float;"
"#endif"
"uniform mediump sampler2D uniSampler2DColor;"
"uniform lowp sampler2D uniSampler2DShadow;"
"uniform lowp float uniDisableShadow[3];"
"uniform lowp float uniSampler2DIris;"
"varying mediump vec2 varTexcoordColor;"
"varying mediump vec4 varTexcoordShadow[3];"
"void main(void){"
    "mediump vec4 v4color=texture2D(uniSampler2DColor,varTexcoordColor);"
    "if(uniDisableShadow[0]==0.0){"
        "vec2 min0=min(abs(varTexcoordShadow[0].xy),vec2(varTexcoordShadow[0].w));"
        "if(varTexcoordShadow[0].xy==min0)"
            "v4color*=texture2DProj(uniSampler2DShadow,varTexcoordShadow[0]).x;"
    "}"
    "if(uniDisableShadow[1]==0.0){"
        "vec2 min1=min(abs(varTexcoordShadow[1].xy),vec2(varTexcoordShadow[1].w));"
        "if(varTexcoordShadow[1].xy==min1)"
            "v4color*=texture2DProj(uniSampler2DShadow,varTexcoordShadow[1]).y;"
    "}"
    "if(uniDisableShadow[2]==0.0){"
        "vec2 min2=min(abs(varTexcoordShadow[2].xy),vec2(varTexcoordShadow[2].w));"
        "if(varTexcoordShadow[2].xy==min2)"
            "v4color*=texture2DProj(uniSampler2DShadow,varTexcoordShadow[2]).z;"
    "}"
    "v4color*=uniSampler2DIris;"
    "gl_FragColor=vec4(v4color.xyz,1.0);"
"}";

static gceSTATUS _Egypt4(gcSHADER Shader)
{
    gceSTATUS status;
    gcUNIFORM
        uniSampler2DColor,
        uniSampler2DShadow,
        uniDisableShadow,
        uniSampler2DIris;
    gcATTRIBUTE
        varTexcoordColor,
        varTexcoordShadow;

    /* uniform mediump sampler2D uniSampler2DColor; */
    ADD_UNIFORM(uniSampler2DColor, SAMPLER_2D);

    /* uniform lowp sampler2D uniSampler2DShadow; */
    ADD_UNIFORM(uniSampler2DShadow, SAMPLER_2D);

    /* uniform lowp float uniDisableShadow[3]; */
    ADD_UNIFORM_ARRAY(uniDisableShadow, FLOAT_X1, 3);

    /* uniform lowp float uniSampler2DIris; */
    ADD_UNIFORM(uniSampler2DIris, FLOAT_X1);

    /* varying mediump vec2 varTexcoordColor; */
    ADD_ATTRIBUTE_TX(varTexcoordColor, FLOAT_X2);

    /* varying mediump vec4 varTexcoordShadow[3]; */
    ADD_ATTRIBUTE_ARRAY_TX(varTexcoordShadow, FLOAT_X4, 3);

    /* void main(void) { */

    /* mediump vec3 v4color = texture2D(uniSampler2DColor, varTexcoordColor);
    **
    **      texld   t0.xyz, uniSampler2DColor, varTexcoordColor.xy
    */
    OPCODE(TEXLD, 0, XYZ); UNIFORM(uniSampler2DColor, XYZW); ATTRIBUTE(varTexcoordColor, XY);

    /* vec3 shadow = vec3(1.0);
    **
    **      mov     t1.xyz, 1
    */
    OPCODE(MOV, 1, XYZ); CONST(1.0f);

    /* if (uniDisableShadow[0] == 0.0) {
    **
    **      jmp.nz  1, uniDisableShadow[0].x
    */
    OPCODE_COND(JMP, NOT_ZERO, 1); UNIFORM_ARRAY(uniDisableShadow, X, 0);

    /* vec2 min0 = min(abs(varTexcoordShadow[0].xy), vec2(varTexcoordShadow[0].w));
    **
    **      abs     t2.xy, varTexcoordShadow[0].xy
    **      min     t3.xy, t2.xy, varTexcoordShadow[0].w
    */
    OPCODE(ABS, 2, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 0);
    OPCODE(MIN, 3, XY); TEMP(2, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, W, 0);

    /* if (varTexcoordShadow[0].xy == min0)
    **
    **      jmp.ne  1, varTexcoordShadow[0].xy, t3.xy
    */
    OPCODE_COND(JMP, NOT_EQUAL, 1); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 0); TEMP(3, XY);

    /* shadow.x = texture2DProj(uniSampler2DShadow, varTexcoordShadow[0]).x;
    **
    **      texldp  t1.x, uniSampler2DShadow, varTexcoordShadow[0]
    */
    OPCODE(TEXLDP, 1, X); UNIFORM(uniSampler2DShadow, XYZW); ATTRIBUTE_ARRAY(varTexcoordShadow, XYZW, 0);

    LABEL(1);
    /* if (uniDisableShadow[1] == 0.0) {
    **
    **      jmp.nz  2, uniDisableShadow[1].x
    */
    OPCODE_COND(JMP, NOT_ZERO, 2); UNIFORM_ARRAY(uniDisableShadow, X, 1);

    /* vec2 min1 = min(abs(varTexcoordShadow[1].xy), vec2(varTexcoordShadow[1].w));
    **
    **      abs     t4.xy, varTexcoordShadow[1].xy
    **      min     t5.xy, t4.xy, varTexcoordShadow[1].w
    */
    OPCODE(ABS, 4, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 1);
    OPCODE(MIN, 5, XY); TEMP(4, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, W, 1);

    /* if (varTexcoordShadow[1].xy == min1)
    **
    **      jmp.ne  2, varTexcoordShadow[1].xy, t5.xy
    */
    OPCODE_COND(JMP, NOT_EQUAL, 2); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 1); TEMP(5, XY);

    /* shadow.y = texture2DProj(uniSampler2DShadow, varTexcoordShadow[1]).y;
    **
    **      texldp  t1.y, uniSampler2DShadow, varTexcoordShadow[1]
    */
    OPCODE(TEXLDP, 1, Y); UNIFORM(uniSampler2DShadow, XYZW); ATTRIBUTE_ARRAY(varTexcoordShadow, XYZW, 1);

    LABEL(2);
    /* if (uniDisableShadow[2] == 0.0) {
    **
    **      jmp.nz  3, uniDisableShadow[2].x
    */
    OPCODE_COND(JMP, NOT_ZERO, 3); UNIFORM_ARRAY(uniDisableShadow, X, 2);

    /* vec2 min2 = min(abs(varTexcoordShadow[2].xy), vec2(varTexcoordShadow[2].w));
    **
    **      abs     t6.xy, varTexcoordShadow[2].xy
    **      min     t7.xy, t6.xy, varTexcoordShadow[2].w
    */
    OPCODE(ABS, 6, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 2);
    OPCODE(MIN, 7, XY); TEMP(6, XY); ATTRIBUTE_ARRAY(varTexcoordShadow, W, 2);

    /* if (varTexcoordShadow[2].xy == min2)
    **
    **      jmp.ne  3, varTexcoordShadow[2].xy, t8.xy
    */
    OPCODE_COND(JMP, NOT_EQUAL, 3); ATTRIBUTE_ARRAY(varTexcoordShadow, XY, 2); TEMP(7, XY);

    /* shadow.x = texture2DProj(uniSampler2DShadow, varTexcoordShadow[2]).x;
    **
    **      texldp  t1.z, uniSampler2DShadow, varTexcoordShadow[2]
    */
    OPCODE(TEXLDP, 1, Z); UNIFORM(uniSampler2DShadow, XYZW); ATTRIBUTE_ARRAY(varTexcoordShadow, XYZW, 2);

    LABEL(3);
    /* v4color *= light * shadow.x * shadow.y * shadow.z * uniSampler2DIris;
    **
    **      mul     t8.xyz, t0.xyz, t1.x
    **      mul     t9.xyz, t8.xyz, t1.y
    **      mul     t10.xyz, t9.xyz, t1.z
    **      mul     t11.xyz, t10.xyz, uniSampler2DIris.x
    */
    OPCODE(MUL,  8, XYZ); TEMP( 0, XYZ); TEMP(1, X);
    OPCODE(MUL,  9, XYZ); TEMP( 8, XYZ); TEMP(1, Y);
    OPCODE(MUL, 10, XYZ); TEMP( 9, XYZ); TEMP(1, Z);
    OPCODE(MUL, 11, XYZ); TEMP(10, XYZ); UNIFORM(uniSampler2DIris, X);

    /* gl_FragColor = vec4(v4color.xyz, 1.0);
    **
    **      mov     t12.xyz, t11.xyz
    **      mov     t12.w, 1
    */
    OPCODE(MOV, 12, XYZ); TEMP(11, XYZ);
    OPCODE(MOV, 12, W); CONST(1.0f);
    COLOR(12);

    /* } */
    PACK();
    return gcvSTATUS_OK;

OnError:
    return status;
}
#endif

lookup compiledShaders[] =
{
    /* Common_test_--balanced vert, v2.0.2. */
    {
        2115,
        "\
#ifdef GL_ES\
precision highp float;\
#endif\
\
#ifdef TEST_BRANCH\
uniform mediump vec2 ccenter;\
#endif\
uniform mediump float time;\
\
attribute vec4 myVertex;\
attribute vec4 myVertex1;\
attribute vec4 myVertex2;\
attribute vec4 myVertex3;\
attribute vec4 myVertex4;\
varying vec4 vVertex;\
varying vec4 vVertex1;\
varying vec4 vVertex2;\
varying vec4 vVertex3;\
varying vec4 vVertex4;\
void main()\
{\
vVertex = myVertex;\
vVertex1 = myVertex1;\
vVertex2 = myVertex2;\
vVertex3 = myVertex3;\
vVertex4 = myVertex4;\
float f=0.;\
float x=myVertex.x;\
float y=myVertex.y;\
float x1=myVertex1.x;\
float y1=myVertex1.y;\
float x2=myVertex2.x;\
float y2=myVertex2.y;\
float x3=myVertex3.x;\
float y3=myVertex3.y;\
float x4=myVertex4.x;\
float y4=myVertex4.y;\
",
"\
float s = mix(clamp(abs(x+time)+abs(y),0.125,0.75),(smoothstep(-0.5,0.5,x)+smoothstep(-0.5,0.5,y))*0.5,(x+1.)*0.5);\
float s1 = mix(clamp(abs(x1+time)+abs(y1),0.125,0.75),(smoothstep(-0.5,0.5,x1)+smoothstep(-0.5,0.5,y1))*0.5,(x1+1.)*0.5);\
float s2 = mix(clamp(abs(x2+time)+abs(y2),0.125,0.75),(smoothstep(-0.5,0.5,x2)+smoothstep(-0.5,0.5,y2))*0.5,(x2+1.)*0.5);\
float s3 = mix(clamp(abs(x3+time)+abs(y3),0.125,0.75),(smoothstep(-0.5,0.5,x3)+smoothstep(-0.5,0.5,y3))*0.5,(x3+1.)*0.5);\
float s4 = mix(clamp(abs(x4+time)+abs(y4),0.125,0.75),(smoothstep(-0.5,0.5,x4)+smoothstep(-0.5,0.5,y4))*0.5,(x4+1.)*0.5);\
float t = mix(clamp(abs(y+time)+abs(x),0.125,0.75),(smoothstep(-0.5,0.5,y)+smoothstep(-0.5,0.5,x))*0.5,(y+1.)*0.5);\
float t1 = mix(clamp(abs(y1+time)+abs(x1),0.125,0.75),(smoothstep(-0.5,0.5,y1)+smoothstep(-0.5,0.5,x1))*0.5,(y1+1.)*0.5);\
float t2 = mix(clamp(abs(y2+time)+abs(x2),0.125,0.75),(smoothstep(-0.5,0.5,y2)+smoothstep(-0.5,0.5,x2))*0.5,(y2+1.)*0.5);\
float t3 = mix(clamp(abs(y3+time)+abs(x3),0.125,0.75),(smoothstep(-0.5,0.5,y3)+smoothstep(-0.5,0.5,x3))*0.5,(y3+1.)*0.5);\
float t4 = mix(clamp(abs(y4+time)+abs(x4),0.125,0.75),(smoothstep(-0.5,0.5,y4)+smoothstep(-0.5,0.5,x4))*0.5,(y4+1.)*0.5);\
f = 1.-(s+s1+s2+s3+s4+t+t1+t2+t3+t4)*0.1;\
\
f*=(1.0-abs(myVertex.x))*(1.0-abs(myVertex.y));\
gl_Position=vec4(myVertex.x,myVertex.y,f,1.+f*0.3333);\
}",
        _Shader1,
    },

    /* Common_test_--balanced vert, v2.0.3. */
    {
        2190,
        "\
#ifdef GL_ES\
precision highp float;\
#ifdef TEST_BRANCH\
uniform mediump vec2 ccenter;\
#endif\
uniform mediump float time;\
#else\
#ifdef TEST_BRANCH\
uniform vec2 ccenter;\
#endif\
uniform float time;\
#endif\
\
attribute vec4 myVertex;\
attribute vec4 myVertex1;\
attribute vec4 myVertex2;\
attribute vec4 myVertex3;\
attribute vec4 myVertex4;\
varying vec4 vVertex;\
varying vec4 vVertex1;\
varying vec4 vVertex2;\
varying vec4 vVertex3;\
varying vec4 vVertex4;\
void main()\
{\
vVertex = myVertex;\
vVertex1 = myVertex1;\
vVertex2 = myVertex2;\
vVertex3 = myVertex3;\
vVertex4 = myVertex4;\
float f=0.;\
float x=myVertex.x;\
float y=myVertex.y;\
float x1=myVertex1.x;\
float y1=myVertex1.y;\
float x2=myVertex2.x;\
float y2=myVertex2.y;\
float x3=myVertex3.x;\
float y3=myVertex3.y;\
float x4=myVertex4.x;\
float y4=myVertex4.y;\
",
"\
float s = mix(clamp(abs(x+time)+abs(y),0.125,0.75),(smoothstep(-0.5,0.5,x)+smoothstep(-0.5,0.5,y))*0.5,(x+1.)*0.5);\
float s1 = mix(clamp(abs(x1+time)+abs(y1),0.125,0.75),(smoothstep(-0.5,0.5,x1)+smoothstep(-0.5,0.5,y1))*0.5,(x1+1.)*0.5);\
float s2 = mix(clamp(abs(x2+time)+abs(y2),0.125,0.75),(smoothstep(-0.5,0.5,x2)+smoothstep(-0.5,0.5,y2))*0.5,(x2+1.)*0.5);\
float s3 = mix(clamp(abs(x3+time)+abs(y3),0.125,0.75),(smoothstep(-0.5,0.5,x3)+smoothstep(-0.5,0.5,y3))*0.5,(x3+1.)*0.5);\
float s4 = mix(clamp(abs(x4+time)+abs(y4),0.125,0.75),(smoothstep(-0.5,0.5,x4)+smoothstep(-0.5,0.5,y4))*0.5,(x4+1.)*0.5);\
float t = mix(clamp(abs(y+time)+abs(x),0.125,0.75),(smoothstep(-0.5,0.5,y)+smoothstep(-0.5,0.5,x))*0.5,(y+1.)*0.5);\
float t1 = mix(clamp(abs(y1+time)+abs(x1),0.125,0.75),(smoothstep(-0.5,0.5,y1)+smoothstep(-0.5,0.5,x1))*0.5,(y1+1.)*0.5);\
float t2 = mix(clamp(abs(y2+time)+abs(x2),0.125,0.75),(smoothstep(-0.5,0.5,y2)+smoothstep(-0.5,0.5,x2))*0.5,(y2+1.)*0.5);\
float t3 = mix(clamp(abs(y3+time)+abs(x3),0.125,0.75),(smoothstep(-0.5,0.5,y3)+smoothstep(-0.5,0.5,x3))*0.5,(y3+1.)*0.5);\
float t4 = mix(clamp(abs(y4+time)+abs(x4),0.125,0.75),(smoothstep(-0.5,0.5,y4)+smoothstep(-0.5,0.5,x4))*0.5,(y4+1.)*0.5);\
f = 1.-(s+s1+s2+s3+s4+t+t1+t2+t3+t4)*0.1;\
\
f*=(1.0-abs(myVertex.x))*(1.0-abs(myVertex.y));\
gl_Position=vec4(myVertex.x,myVertex.y,f,1.+f*0.3333);\
}",
        _Shader1,
    },

    /* Common_test_--balanced frag, v2.0.2. */
    {
        1795,
        "#ifdef GL_ES\
precision mediump float;\
#endif\
\
#ifdef TEST_BRANCH\
uniform vec2 ccenter;\
#endif\
uniform float time;\
\
varying vec4 vVertex;\
varying vec4 vVertex1;\
varying vec4 vVertex2;\
varying vec4 vVertex3;\
varying vec4 vVertex4;\
\
void main()\
{\
float f=0.;\
float x=vVertex.x;\
float y=vVertex.y;\
float x1=vVertex1.x;\
float y1=vVertex1.y;\
float x2=vVertex2.x;\
float y2=vVertex2.y;\
float x3=vVertex3.x;\
float y3=vVertex3.y;\
float x4=vVertex4.x;\
float y4=vVertex4.y;\
\
float s=mix(clamp(abs(x+time)+abs(y),0.125,0.75),(smoothstep(-0.5,0.5,x)+smoothstep(-0.5,0.5,y))*0.5,(x+1.)*0.5);\
float s1=mix(clamp(abs(x1+time)+abs(y1),0.125,0.75),(smoothstep(-0.5,0.5,x1)+smoothstep(-0.5,0.5,y1))*0.5,(x1+1.)*0.5);\
float s2=mix(clamp(abs(x2+time)+abs(y2),0.125,0.75),(smoothstep(-0.5,0.5,x2)+smoothstep(-0.5,0.5,y2))*0.5,(x2+1.)*0.5);\
float s3=mix(clamp(abs(x3+time)+abs(y3),0.125,0.75),(smoothstep(-0.5,0.5,x3)+smoothstep(-0.5,0.5,y3))*0.5,(x3+1.)*0.5);\
float s4=mix(clamp(abs(x4+time)+abs(y4),0.125,0.75),(smoothstep(-0.5,0.5,x4)+smoothstep(-0.5,0.5,y4))*0.5,(x4+1.)*0.5);\
float t=mix(clamp(abs(y+time)+abs(x),0.125,0.75),(smoothstep(-0.5,0.5,y)+smoothstep(-0.5,0.5,x))*0.5,(y+1.)*0.5);\
float t1=mix(clamp(abs(y1+time)+abs(x1),0.125,0.75),(smoothstep(-0.5,0.5,y1)+smoothstep(-0.5,0.5,x1))*0.5,(y1+1.)*0.5);\
float t2=mix(clamp(abs(y2+time)+abs(x2),0.125,0.75),(smoothstep(-0.5,0.5,y2)+smoothstep(-0.5,0.5,x2))*0.5,(y2+1.)*0.5);\
float t3=mix(clamp(abs(y3+time)+abs(x3),0.125,0.75),(smoothstep(-0.5,0.5,y3)+smoothstep(-0.5,0.5,x3))*0.5,(y3+1.)*0.5);\
float t4=mix(clamp(abs(y4+time)+abs(x4),0.125,0.75),(smoothstep(-0.5,0.5,y4)+smoothstep(-0.5,0.5,x4))*0.5,(y4+1.)*0.5);\
f=1.-(s+s1+s2+s3+s4+t+t1+t2+t3+t4)*0.1;\
\
gl_FragColor=vec4(vVertex.x,vVertex.y,f,1.+f*0.3333);\
}",
        gcvNULL,
        _Shader2,
	},

    {  604, _Egypt0Shader, gcvNULL, _Egypt0 },
    {  700, _Egypt1Shader, gcvNULL, _Egypt1 },
    { 1738, _Egypt2Shader, gcvNULL, _Egypt2 },
#if EGYPT3
    { 1215, _Egypt3Shader, gcvNULL, _Egypt3 }
#endif
#if EGYPT4
    { 1078, _Egypt4Shader, gcvNULL, _Egypt4 },
#endif

    { 0, gcvNULL, gcvNULL, gcvNULL }
};

#endif /* __gc_glsh_precompiled_shaer_objects_ */

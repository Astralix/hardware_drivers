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




#include "gc_hal_user_hardware_precomp.h"
#include "gc_hal_compiler.h"

#ifndef VIVANTE_NO_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE        gcvZONE_COMPOSITION
#define _ENABLE_DUMPING     0

/******************************************************************************\
***************************** Internal Definitions *****************************
\******************************************************************************/

/* Number of target info items per container. */
#define gcdiTARGET_ITEM_COUNT   16

/* Number of node info items per container. */
#define gcdiNODE_ITEM_COUNT     32

/* Number of layer info items per container. */
#define gcdiLAYER_ITEM_COUNT    32

/* Blurring kernel size. */
#define gcdiBLUR_KERNEL_SIZE    9
#define gcdiBLUR_KERNEL_HALF    (gcdiBLUR_KERNEL_SIZE / 2 + 1)

/***************************************************************************************************/

#define gcmiCOMPOSITIONSTATE(Buffer, Address, Data)                                                 \
                  (Buffer)->tail  [0]  = Address;                                                   \
                  (Buffer)->tail  [1]  = Data;                                                      \
                  (Buffer)->tail       += 2;                                                        \
                  (Buffer)->available  -= 2 * gcmSIZEOF(gctUINT32);                                 \
                  (Buffer)->count      += 1

#define gcmiCOMPOSITIONFLOATCONST(Resources, Buffer, Address, Data)                                 \
                  (Buffer)->tail  [0]  = Address + Resources->constStateBase;                       \
    ((gctFLOAT *) (Buffer)->tail) [1]  = Data;                                                      \
                  (Buffer)->tail       += 2;                                                        \
                  (Buffer)->available  -= 2 * gcmSIZEOF(gctUINT32);                                 \
                  (Buffer)->count      += 1

#define gcmiCOMPOSITIONCONST(Resources, Buffer, Address, Data)                                      \
                  (Buffer)->tail  [0]  = Address + Resources->constStateBase;                       \
                  (Buffer)->tail  [1]  = Data;                                                      \
                  (Buffer)->tail       += 2;                                                        \
                  (Buffer)->available  -= 2 * gcmSIZEOF(gctUINT32);                                 \
                  (Buffer)->count      += 1

/***************************************************************************************************/

#define gcmiSOURCEFORMAT                                                                            \
    "%s%s%d%s"

#define gcmiSOURCENEG(Negate)                                                                       \
    (Negate) ? "-" : ""

#define gcmiSOURCETYPE(Type)                                                                        \
    (AQ_SHADER_SRC_REG_TYPE_ ## Type == 0x0)                                \
        ? "r"                                                                                       \
        : (AQ_SHADER_SRC_REG_TYPE_ ## Type == 0x2)               \
            ? "c"                                                                                   \
            : "<?>"

#define gcmiSOURCESWIZZLE(SourceSwizzle)                                                            \
      (SourceSwizzle == gcSL_SWIZZLE_XYZW) ? ""                                                     \
    : (SourceSwizzle == gcSL_SWIZZLE_XXXX) ? ".x"                                                   \
    : (SourceSwizzle == gcSL_SWIZZLE_YYYY) ? ".y"                                                   \
    : (SourceSwizzle == gcSL_SWIZZLE_ZZZZ) ? ".z"                                                   \
    : (SourceSwizzle == gcSL_SWIZZLE_WWWW) ? ".w"                                                   \
    : (SourceSwizzle == gcSL_SWIZZLE_XYYY) ? ".xy"                                                  \
    :                                        ".????"                                                \

#define gcmiSOURCE(SourceType, SourceAddress, SourceSwizzle, SourceNegate)                          \
    gcmiSOURCENEG(SourceNegate), \
    gcmiSOURCETYPE(SourceType), \
    SourceAddress, \
    gcmiSOURCESWIZZLE(SourceSwizzle)

/***************************************************************************************************/

#define gcmiCOMPOSITON_MOV(\
        Buffer, \
        SourceType, SourceAddress, SourceSwizzle, SourceMod, \
        Destination                                                                                 \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] MOV r%d, " gcmiSOURCEFORMAT "\n", \
        __FUNCTION__, __LINE__, \
        Resources->currPC, \
        Destination, \
        gcmiSOURCE(SourceType, SourceAddress, SourceSwizzle, SourceMod)                             \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, MOV)                                      \
                                                                                                    \
        /* Destination. */                                                                          \
        | gcmSETFIELD     (0, AQ_INST, DEST_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, DEST_ADR, Destination)                              \
        | gcmSETFIELD     (0, AQ_INST, DEST_WRITE_ENABLE, 0xF), \
                                                                                                    \
          0, 0, \
                                                                                                    \
          gcmSETFIELD     (0, AQ_INST, SRC2_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC2_ADR, SourceAddress)                            \
        | gcmSETFIELD     (0, AQ_INST, SRC2_SWIZZLE, SourceSwizzle)                            \
        | gcmSETFIELD     (0, AQ_INST, SRC2_MODIFIER_NEG, SourceMod)                                \
        | gcmSETFIELD     (0, AQ_INST, SRC2_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## SourceType)    \
        ))

#define gcmiCOMPOSITON_ADD(\
        Buffer, \
        Source1Type, Source1Address, Source1Swizzle, Source1Mod, \
        Source2Type, Source2Address, Source2Swizzle, Source2Mod, \
        Destination, DestinationEnable                                                              \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] ADD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n", \
        __FUNCTION__, __LINE__, \
        Resources->currPC, \
        Destination, \
        gcmiSOURCE(Source1Type, Source1Address, Source1Swizzle, Source1Mod), \
        gcmiSOURCE(Source2Type, Source2Address, Source2Swizzle, Source2Mod)                         \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, ADD)                                      \
                                                                                                    \
        /* Destination. */                                                                          \
        | gcmSETFIELD     (0, AQ_INST, DEST_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, DEST_ADR, Destination)                              \
        | gcmSETFIELD     (0, AQ_INST, DEST_WRITE_ENABLE, DestinationEnable), \
                                                                                                    \
        /* Source1. */                                                                              \
          gcmSETFIELD     (0, AQ_INST, SRC0_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC0_ADR, Source1Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC0_SWIZZLE, Source1Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC0_MODIFIER_NEG, Source1Mod), \
          gcmSETFIELD     (0, AQ_INST, SRC0_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Source1Type), \
                                                                                                    \
        /* Source2. */                                                                              \
          gcmSETFIELD     (0, AQ_INST, SRC2_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC2_ADR, Source2Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC2_SWIZZLE, Source2Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC2_MODIFIER_NEG, Source2Mod)                               \
        | gcmSETFIELD     (0, AQ_INST, SRC2_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Source2Type)   \
        ))

#define gcmiCOMPOSITON_MUL(\
        Buffer, \
        Source1Type, Source1Address, Source1Swizzle, Source1Mod, \
        Source2Type, Source2Address, Source2Swizzle, Source2Mod, \
        Destination                                                                                 \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] MUL r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n", \
        __FUNCTION__, __LINE__, \
        Resources->currPC, \
        Destination, \
        gcmiSOURCE(Source1Type, Source1Address, Source1Swizzle, Source1Mod), \
        gcmiSOURCE(Source2Type, Source2Address, Source2Swizzle, Source2Mod)                         \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, MUL)                                      \
                                                                                                    \
        /* Destination. */                                                                          \
        | gcmSETFIELD     (0, AQ_INST, DEST_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, DEST_ADR, Destination)                              \
        | gcmSETFIELD     (0, AQ_INST, DEST_WRITE_ENABLE, 0xF), \
                                                                                                    \
        /* Temp. */                                                                                 \
          gcmSETFIELD     (0, AQ_INST, SRC0_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC0_ADR, Source1Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC0_SWIZZLE, Source1Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC0_MODIFIER_NEG, Source1Mod), \
          gcmSETFIELD     (0, AQ_INST, SRC0_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Source1Type)   \
                                                                                                    \
        /* Const. */                                                                                \
        | gcmSETFIELD     (0, AQ_INST, SRC1_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC1_ADR, Source2Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC1_SWIZZLE, Source2Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC1_MODIFIER_NEG, Source2Mod), \
          gcmSETFIELD     (0, AQ_INST, SRC1_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Source2Type)   \
        ))

#define gcmiCOMPOSITON_MAD(\
        Buffer, \
        Source1Type, Source1Address, Source1Swizzle, Source1Mod, Source1Relative, \
        Source2Type, Source2Address, Source2Swizzle, Source2Mod, Source2Relative, \
        Source3Type, Source3Address, Source3Swizzle, Source3Mod, Source3Relative, \
        Destination                                                                                 \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] MAD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n", \
        __FUNCTION__, __LINE__, \
        Resources->currPC, \
        Destination, \
        gcmiSOURCE(Source1Type, Source1Address, Source1Swizzle, Source1Mod), \
        gcmiSOURCE(Source2Type, Source2Address, Source2Swizzle, Source2Mod), \
        gcmiSOURCE(Source3Type, Source3Address, Source3Swizzle, Source3Mod)                         \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, MAD)                                      \
                                                                                                    \
        /* Destination. */                                                                          \
        | gcmSETFIELD     (0, AQ_INST, DEST_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, DEST_ADR, Destination)                              \
        | gcmSETFIELD     (0, AQ_INST, DEST_WRITE_ENABLE, 0xF), \
                                                                                                    \
        /* Source1. */                                                                              \
          gcmSETFIELD     (0, AQ_INST, SRC0_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC0_ADR, Source1Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC0_SWIZZLE, Source1Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC0_MODIFIER_NEG, Source1Mod), \
          gcmSETFIELD     (0, AQ_INST, SRC0_REL_ADR, AQ_SHADER_REL_ADR_ ## Source1Relative)    \
        | gcmSETFIELD     (0, AQ_INST, SRC0_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Source1Type)   \
                                                                                                    \
        /* Source2. */                                                                              \
        | gcmSETFIELD     (0, AQ_INST, SRC1_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC1_ADR, Source2Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC1_SWIZZLE, Source2Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC1_MODIFIER_NEG, Source2Mod)                               \
        | gcmSETFIELD     (0, AQ_INST, SRC1_REL_ADR, AQ_SHADER_REL_ADR_ ## Source2Relative), \
          gcmSETFIELD     (0, AQ_INST, SRC1_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Source2Type)   \
                                                                                                    \
        /* Source3. */                                                                              \
        | gcmSETFIELD     (0, AQ_INST, SRC2_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC2_ADR, Source3Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC2_SWIZZLE, Source3Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC2_MODIFIER_NEG, Source3Mod)                               \
        | gcmSETFIELD     (0, AQ_INST, SRC2_REL_ADR, AQ_SHADER_REL_ADR_ ## Source3Relative)    \
        | gcmSETFIELD     (0, AQ_INST, SRC2_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Source3Type)   \
        ))

#define gcmiCOMPOSITON_TEXLD(\
        Buffer, \
        Sampler, Coordinates, Destination                                                           \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] TEXLD r%d, s%d, " gcmiSOURCEFORMAT "\n", \
        __FUNCTION__, __LINE__, \
        Resources->currPC, \
        Destination, Sampler, \
        gcmiSOURCE(TEMP, Coordinates, gcSL_SWIZZLE_XYYY, 0)                                         \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, TEXLD)                                    \
                                                                                                    \
        /* Destination. */                                                                          \
        | gcmSETFIELD     (0, AQ_INST, DEST_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, DEST_ADR, Destination)                              \
        | gcmSETFIELD     (0, AQ_INST, DEST_WRITE_ENABLE, 0xF)                                      \
                                                                                                    \
        /* Sampler. */                                                                              \
        | gcmSETFIELD     (0, AQ_INST, SAMPLER_NUM, Sampler), \
          gcmSETFIELD     (0, AQ_INST, SAMPLER_SWIZZLE, gcSL_SWIZZLE_XYZW)                        \
                                                                                                    \
        /* Coord.xy */                                                                              \
        | gcmSETFIELD     (0, AQ_INST, SRC0_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC0_ADR, Coordinates)                              \
        | gcmSETFIELD     (0, AQ_INST, SRC0_SWIZZLE, gcSL_SWIZZLE_XYYY)                        \
        | gcmSETFIELD     (0, AQ_INST, SRC0_MODIFIER_NEG, 0), \
          gcmSETFIELD     (0, AQ_INST, SRC0_TYPE, 0x0), \
                                                                                                    \
          0                                                                                         \
        ))

#define gcmiCOMPOSITON_SELECT(\
        Buffer, \
        Operation, \
        CheckType, CheckAddress, CheckSwizzle, CheckMod, \
        Select1Type, Select1Address, Select1Swizzle, Select1Mod, \
        Select2Type, Select2Address, Select2Swizzle, Select2Mod, \
        Destination, DestinationEnable                                                              \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] SELECT." #Operation " r%d, "                                                  \
                      gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "d\n", \
        __FUNCTION__, __LINE__, \
        Resources->currPC, \
        Destination, \
        gcmiSOURCE(CheckType, CheckAddress, CheckSwizzle, CheckMod), \
        gcmiSOURCE(Select1Type, Select1Address, Select1Swizzle, Select1Mod), \
        gcmiSOURCE(Select2Type, Select2Address, Select2Swizzle, Select2Mod)                         \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, SELECT)                                   \
                                                                                                    \
        /* Destination. */                                                                          \
        | gcmSETFIELD     (0, AQ_INST, DEST_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, DEST_ADR, Destination)                              \
        | gcmSETFIELD     (0, AQ_INST, DEST_WRITE_ENABLE, DestinationEnable)                        \
        | gcmSETFIELDVALUE(0, AQ_INST, CONDITION_CODE, Operation), \
                                                                                                    \
        /* Check value. */                                                                          \
          gcmSETFIELD     (0, AQ_INST, SRC0_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC0_ADR, CheckAddress)                             \
        | gcmSETFIELD     (0, AQ_INST, SRC0_SWIZZLE, CheckSwizzle)                             \
        | gcmSETFIELD     (0, AQ_INST, SRC0_MODIFIER_NEG, CheckMod), \
          gcmSETFIELD     (0, AQ_INST, SRC0_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## CheckType)     \
                                                                                                    \
        /* Select1 */                                                                               \
        | gcmSETFIELD     (0, AQ_INST, SRC1_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC1_ADR, Select1Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC1_SWIZZLE, Select1Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC1_MODIFIER_NEG, Select1Mod), \
          gcmSETFIELD     (0, AQ_INST, SRC1_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Select1Type)   \
                                                                                                    \
        /* Select2 */                                                                               \
        | gcmSETFIELD     (0, AQ_INST, SRC2_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC2_ADR, Select2Address)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC2_SWIZZLE, Select2Swizzle)                           \
        | gcmSETFIELD     (0, AQ_INST, SRC2_MODIFIER_NEG, Select2Mod)                               \
        | gcmSETFIELD     (0, AQ_INST, SRC2_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## Select2Type)   \
        ))

#define gcmiCOMPOSITON_LOOP(\
        Buffer, \
        InfoType, InfoAddress, InfoSwizzle, InfoMod, \
        Label                                                                                       \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] REP\n", \
        Resources->currPC, \
        __FUNCTION__, __LINE__                                                                      \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, LOOP), \
                                                                                                    \
          0, \
                                                                                                    \
        /* Loop info. */                                                                            \
          gcmSETFIELD     (0, AQ_INST, SRC1_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC1_ADR, InfoAddress)                              \
        | gcmSETFIELD     (0, AQ_INST, SRC1_SWIZZLE, InfoSwizzle)                              \
        | gcmSETFIELD     (0, AQ_INST, SRC1_MODIFIER_NEG, InfoMod), \
          gcmSETFIELD     (0, AQ_INST, SRC1_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## InfoType)      \
                                                                                                    \
        /* Label. */                                                                                \
        | gcmSETFIELD     (0, AQ_INST, TARGET, Label)                                    \
        ))

#define gcmiCOMPOSITON_ENDLOOP(\
        Buffer, \
        InfoType, InfoAddress, InfoSwizzle, InfoMod, \
        RepeatLabel                                                                                 \
        )                                                                                           \
                                                                                                    \
    gcmTRACE_ZONE(\
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION, \
        "%s(%d): [%d] ENDREP\n", \
        __FUNCTION__, __LINE__, \
        Resources->currPC                                                                           \
        );                                                                                          \
                                                                                                    \
    gcmONERROR(_SetShader(\
        Hardware, Resources, Buffer, \
                                                                                                    \
          gcmSETFIELDVALUE(0, AQ_INST, OP_CODE, ENDLOOP), \
                                                                                                    \
          0, \
                                                                                                    \
        /* Loop info. */                                                                            \
          gcmSETFIELD     (0, AQ_INST, SRC1_VALID, 1)                                        \
        | gcmSETFIELD     (0, AQ_INST, SRC1_ADR, InfoAddress)                              \
        | gcmSETFIELD     (0, AQ_INST, SRC1_SWIZZLE, InfoSwizzle)                              \
        | gcmSETFIELD     (0, AQ_INST, SRC1_MODIFIER_NEG, InfoMod), \
          gcmSETFIELD     (0, AQ_INST, SRC1_TYPE, AQ_SHADER_SRC_REG_TYPE_ ## InfoType)      \
                                                                                                    \
        /* RepeatLabel. */                                                                          \
        | gcmSETFIELD     (0, AQ_INST, TARGET, RepeatLabel)                              \
        ))

typedef struct _gcsiCOMPOSITION_RESOURCES * gcsiCOMPOSITION_RESOURCES_PTR;
typedef struct _gcsiCOMPOSITION_RESOURCES
{
    /* Current sampler. */
    gctUINT                         currSampler;

    /* Constant base addresses. */
    gctUINT                         constStateBase;
    gctUINT                         constOffset;

    /* Generic constant allocator. */
    gctUINT                         constAllocator;

    /* Current constant and the state address. */
    gctINT                          nextClearValue;
    gctINT                          nextAlphaValue;

    /* Current and previous temporary registers. */
    gctINT                          prevTemp;
    gctINT                          currTemp;

    /* Current shader PC and the state address. */
    gctUINT                         currPC;
    gctUINT                         currPCaddress;
}
gcsiCOMPOSITION_RESOURCES;

typedef struct _gcsiCONST_ALLOCATOR * gcsiCONST_ALLOCATOR_PTR;
typedef struct _gcsiCONST_ALLOCATOR
{
    gctUINT                         index;
    gctUINT                         address;
    gctUINT                         swizzle;
}
gcsiCONST_ALLOCATOR;

typedef struct _gcsiFILTER_PARAMETERS * gcsiFILTER_PARAMETERS_PTR;
typedef struct _gcsiFILTER_PARAMETERS
{
    gcsiCONST_ALLOCATOR             loop1Info;
    gcsiCONST_ALLOCATOR             loop2Info;

    gcsiCONST_ALLOCATOR             stepX;
    gcsiCONST_ALLOCATOR             stepY;

    gcsiCONST_ALLOCATOR             zero;
    gcsiCONST_ALLOCATOR             one;

    gcsiCONST_ALLOCATOR             coefficient0;
    gcsiCONST_ALLOCATOR             coefficient1;
    gcsiCONST_ALLOCATOR             coefficient2;
    gcsiCONST_ALLOCATOR             coefficient3;
    gcsiCONST_ALLOCATOR             coefficient4;
}
gcsiFILTER_PARAMETERS;

static gctUINT _compSwizzle[] =
{
    gcSL_SWIZZLE_XXXX,
    gcSL_SWIZZLE_YYYY,
    gcSL_SWIZZLE_ZZZZ,
    gcSL_SWIZZLE_WWWW
};

/******************************************************************************\
******************************* Internal Functions *****************************
\******************************************************************************/

static gceSTATUS
_TranslateTargetFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT Format,
    OUT gctUINT32 * TargetFormat,
    OUT gctUINT32 * TargetSwizzle,
    OUT gctBOOL * HasAlpha
    )
{
    switch (Format)
    {
    case gcvSURF_X4R4G4B4:
        *TargetFormat = 0x0;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvFALSE;
        return gcvSTATUS_OK;

    case gcvSURF_A4R4G4B4:
        *TargetFormat = 0x1;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvTRUE;
        return gcvSTATUS_OK;

    case gcvSURF_X1R5G5B5:
        *TargetFormat = 0x2;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvFALSE;
        return gcvSTATUS_OK;

    case gcvSURF_A1R5G5B5:
        *TargetFormat = 0x3;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvTRUE;
        return gcvSTATUS_OK;

    case gcvSURF_R5G6B5:
        *TargetFormat = 0x4;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvFALSE;
        return gcvSTATUS_OK;

    case gcvSURF_YUY2:
        if (((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 24:24) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))))
        {
            *TargetFormat = 0x7;
            *TargetSwizzle
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
            *HasAlpha = gcvFALSE;
            return gcvSTATUS_OK;
        }
        else
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): YUY2 render target is not supported.\n",
                __FUNCTION__, __LINE__
                );

            return gcvSTATUS_INVALID_ARGUMENT;
        }

    case gcvSURF_X8R8G8B8:
        *TargetFormat = 0x5;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvFALSE;
        return gcvSTATUS_OK;

    case gcvSURF_A8R8G8B8:
        *TargetFormat = 0x6;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvTRUE;
        return gcvSTATUS_OK;

    case gcvSURF_A8B8G8R8:
        *TargetFormat = 0x6;
        *TargetSwizzle
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)));
        *HasAlpha = gcvTRUE;
        return gcvSTATUS_OK;

    default:
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): Unknown target format: %d.\n",
            __FUNCTION__, __LINE__, Format
            );

        return gcvSTATUS_INVALID_ARGUMENT;
    }
}

static gceSTATUS
_TranslateSourceFormat(
    IN gceSURF_FORMAT Format,
    OUT gctUINT32_PTR SourceFormat,
    OUT gctBOOL * HasAlpha
    )
{
    switch (Format)
    {
    case gcvSURF_A8:
        * SourceFormat = 0x01;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_L8:
        * SourceFormat = 0x02;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_A8L8:
        * SourceFormat = 0x04;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_X4R4G4B4:
        * SourceFormat = 0x06;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_A4R4G4B4:
        * SourceFormat = 0x05;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_X1R5G5B5:
        * SourceFormat = 0x0D;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_A1R5G5B5:
        * SourceFormat = 0x0C;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_R5G6B5:
        * SourceFormat = 0x0B;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_X8R8G8B8:
        * SourceFormat = 0x08;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_A8R8G8B8:
        * SourceFormat = 0x07;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_X8B8G8R8:
        * SourceFormat = 0x0A;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_A8B8G8R8:
        * SourceFormat = 0x09;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_YUY2:
        * SourceFormat = 0x0E;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_UYVY:
        * SourceFormat = 0x0F;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_D16:
        * SourceFormat = 0x10;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_D24S8:
        * SourceFormat = 0x11;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_D24X8:
        * SourceFormat = 0x11;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_DXT1:
        * SourceFormat = 0x13;
        * HasAlpha     = gcvFALSE;
        break;

    case gcvSURF_DXT2:
        * SourceFormat = 0x14;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_DXT3:
        * SourceFormat = 0x14;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_DXT4:
        * SourceFormat = 0x15;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_DXT5:
        * SourceFormat = 0x15;
        * HasAlpha     = gcvTRUE;
        break;

    case gcvSURF_ETC1:
        * SourceFormat = 0x1E;
        * HasAlpha     = gcvFALSE;
        break;

    default:
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): Unknown source format: %d.\n",
            __FUNCTION__, __LINE__, Format
            );

        goto OnError;
    }

    return gcvSTATUS_OK;

OnError:
    return gcvSTATUS_NOT_SUPPORTED;
}

static gceSTATUS
_TranslateCompositionTileStatusFormat(
    IN gceSURF_FORMAT Format,
    OUT gctUINT32_PTR TileStatusFormat
    )
{
    switch (Format)
    {
    case gcvSURF_A4R4G4B4:
        * TileStatusFormat = 0x0;
        break;

    case gcvSURF_A1R5G5B5:
        * TileStatusFormat = 0x1;
        break;

    case gcvSURF_R5G6B5:
        * TileStatusFormat = 0x2;
        break;

    case gcvSURF_A8R8G8B8:
    case gcvSURF_A8B8G8R8:
        * TileStatusFormat = 0x3;
        break;

    case gcvSURF_X8R8G8B8:
        * TileStatusFormat = 0x4;
        break;

    case gcvSURF_D24S8:
        * TileStatusFormat = 0x5;
        break;

    case gcvSURF_D24X8:
        * TileStatusFormat = 0x6;
        break;

    default:
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): Unknown tile status format: %d.\n",
            __FUNCTION__, __LINE__, Format
            );

        goto OnError;
    }

    return gcvSTATUS_OK;

OnError:
    return gcvSTATUS_NOT_SUPPORTED;
}

static gceSTATUS
_AppendLayer(
    IN gcoHARDWARE Hardware,
    IN gcsCOMPOSITION_SET_PTR Set,
    IN gcsCOMPOSITION_LAYER_PTR Layer
    )
{
    gceSTATUS status;
    gcsCOMPOSITION_NODE_PTR tempNode;

    /* Allocate new layer wrapper node. */
    gcmONERROR(gcsCONTAINER_AllocateRecord(
        &Hardware->composition.freeNodes,
        (gctPOINTER *) &tempNode
        ));

    /* Add the node to the set. */
    Set->nodeTail->next = tempNode;
    Set->nodeTail       = tempNode;
    tempNode->next      = gcvNULL;

    /* Set the layer. */
    tempNode->layer = Layer;

OnError:
    /* Return status. */
    return status;
}

static gceSTATUS
_CopySet(
    IN gcoHARDWARE Hardware,
    IN gcsCOMPOSITION_SET_PTR TemplateSet,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom
    )
{
    gceSTATUS status;
    gcsCOMPOSITION_SET_PTR tempSet;
    gcsCOMPOSITION_NODE_PTR tempNode;
    gcsCOMPOSITION_NODE_PTR node;

    gcmONERROR(gcsCONTAINER_AllocateRecord(
        &Hardware->composition.freeSets,
        (gctPOINTER *) &tempSet
        ));

    tempSet->blur = gcvFALSE;

    tempSet->trgRect.left   = Left;
    tempSet->trgRect.right  = Right;
    tempSet->trgRect.top    = Top;
    tempSet->trgRect.bottom = Bottom;

    tempSet->nodeHead =
    tempSet->nodeTail = gcvNULL;

    /* Add to the list of sets. */
    tempSet->prev = TemplateSet;
    tempSet->next = TemplateSet->next;
    TemplateSet->next->prev = tempSet;
    TemplateSet->next       = tempSet;

    /* Copy all layer nodes. */
    node = TemplateSet->nodeHead;
    while (node != gcvNULL)
    {
        gcmONERROR(gcsCONTAINER_AllocateRecord(
            &Hardware->composition.freeNodes,
            (gctPOINTER *) &tempNode
            ));

        tempNode->layer = node->layer;
        tempNode->next  = gcvNULL;

        if (tempSet->nodeHead == gcvNULL)
        {
            tempSet->nodeHead = tempNode;
        }
        else
        {
            tempSet->nodeTail->next = tempNode;
        }

        tempSet->nodeTail = tempNode;

        node = node->next;
    }

OnError:
    /* Return status. */
    return status;
}

static gceSTATUS
_ProcessLayer(
    IN gcoHARDWARE Hardware,
    IN gcsCOMPOSITION_LAYER_PTR Layer,
    IN gcsRECT_PTR TargetRect,
    IN gcsCOMPOSITION_SET_PTR FirstSet
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCOMPOSITION_SET_PTR tempSet;
    gcsCOMPOSITION_SET_PTR currSet;
    gcsCOMPOSITION_SET_PTR nextSet;
    gcsCOMPOSITION_SET_PTR overlappingSet;
    gcsCOMPOSITION_NODE_PTR tempNode;
    gctINT32 x1, x2;
    gctINT32 y1, y2;
    gcsRECT layerRect;

    if ((Hardware->composition.head.next == &Hardware->composition.head) ||
        (Layer->input.operation == gcvCOMPOSE_BLUR))
    {
        /* Allocate new set. */
        gcmONERROR(gcsCONTAINER_AllocateRecord(
            &Hardware->composition.freeSets,
            (gctPOINTER *) &tempSet
            ));

        /* Add to the list. */
        if (Hardware->composition.head.next == &Hardware->composition.head)
        {
            tempSet->prev = &Hardware->composition.head;
            tempSet->next = &Hardware->composition.head;
            Hardware->composition.head.next = tempSet;
            Hardware->composition.head.prev = tempSet;
        }
        else
        {
            tempSet->prev =  Hardware->composition.head.prev;
            tempSet->next = &Hardware->composition.head;
            Hardware->composition.head.prev->next = tempSet;
            Hardware->composition.head.prev       = tempSet;
        }

        /* Initialize the set. */
        tempSet->blur     = (Layer->input.operation == gcvCOMPOSE_BLUR);
        tempSet->trgRect  = *TargetRect;
        tempSet->nodeHead =
        tempSet->nodeTail = gcvNULL;

        /* Allocate new layer node. */
        gcmONERROR(gcsCONTAINER_AllocateRecord(
            &Hardware->composition.freeNodes,
            (gctPOINTER *) &tempNode
            ));

        /* Add to the list. */
        tempSet->nodeHead =
        tempSet->nodeTail = tempNode;

        /* Initialize the node. */
        tempNode->layer = Layer;
        tempNode->next  = gcvNULL;
    }
    else
    {
        currSet = (FirstSet == gcvNULL)
            ? Hardware->composition.head.next
            : FirstSet;

        while (currSet != &Hardware->composition.head)
        {
            /*******************************************************************
            ** Blur layers cannot be combined with other layers, skip.
            */

            if (currSet->blur)
            {
                currSet = currSet->next;
                continue;
            }

            /*******************************************************************
            ** Compute overlap with the current target.
            */

            x1 = gcmMAX(currSet->trgRect.left, TargetRect->left);
            x2 = gcmMIN(currSet->trgRect.right, TargetRect->right);

            if (x1 >= x2)
            {
                /* No overlap, go to the next trarget. */
                currSet = currSet->next;
                continue;
            }

            y1 = gcmMAX(currSet->trgRect.top, TargetRect->top);
            y2 = gcmMIN(currSet->trgRect.bottom, TargetRect->bottom);

            if (y1 >= y2)
            {
                /* No overlap, go to the next trarget. */
                currSet = currSet->next;
                continue;
            }

            /*******************************************************************
            ** Analyze the overlap: current layer set side.
            */

            /* Store the pointer to the next layer. */
            nextSet = currSet->next;

            if (x1 == currSet->trgRect.left)
            {
                if (x2 == currSet->trgRect.right)
                {
                    if (y1 == currSet->trgRect.top)
                    {
                        if (y2 == currSet->trgRect.bottom)
                        {
                            /*
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             */

                            if (Layer->needPrevious)
                            {
                                /* Set a pointer to the overlapping region. */
                                overlappingSet = currSet;

                                /* Add the layer to the overlapping region. */
                                gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                            }
                            else
                            {
                                /* This set is completely covered by the opaque layer -
                                   remove the entire set. */
                                currSet->prev->next = currSet->next;
                                currSet->next->prev = currSet->prev;
                            }
                        }
                        else
                        {
                            /*
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             */

                            if (Layer->needPrevious)
                            {
                                /* Create a new target for the bottom non-overlapping region. */
                                gcmONERROR(_CopySet(
                                    Hardware, currSet,
                                    x1, y2,
                                    x2, currSet->trgRect.bottom
                                    ));

                                /* Set a pointer to the overlapping region. */
                                overlappingSet = currSet;

                                /* Correct coordinates of the top overlapping region. */
                                currSet->trgRect.bottom = y2;

                                /* Add the layer to the overlapping region. */
                                gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                            }
                            else
                            {
                                /* The overlapping region is covered by the opaque layer -
                                   remove the overlapping region. */
                                currSet->trgRect.top = y2;
                            }
                        }
                    }
                    else if (y2 == currSet->trgRect.bottom)
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * | X | X | X |
                         * +---+---+---+
                         * | X | X | X |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the bottom overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the top non-overlapping region. */
                            currSet->trgRect.bottom = y1;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */
                            currSet->trgRect.bottom = y1;
                        }
                    }
                    else
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * | X | X | X |
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y2,
                                x2, currSet->trgRect.bottom
                                ));

                            /* Create a new target for the middle overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the top non-overlapping region. */
                            currSet->trgRect.bottom = y1;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */

                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y2,
                                x2, currSet->trgRect.bottom
                                ));

                            /* Correct coordinates of the top non-overlapping region. */
                            currSet->trgRect.bottom = y1;
                        }
                    }
                }
                else
                {
                    if (y1 == currSet->trgRect.top)
                    {
                        if (y2 == currSet->trgRect.bottom)
                        {
                            /*
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             */

                            if (Layer->needPrevious)
                            {
                                /* Create a new target for the right non-overlapping region. */
                                gcmONERROR(_CopySet(
                                    Hardware, currSet,
                                    x2, currSet->trgRect.top,
                                    currSet->trgRect.right, currSet->trgRect.bottom
                                    ));

                                /* Set a pointer to the overlapping region. */
                                overlappingSet = currSet;

                                /* Correct coordinates of the left overlapping region. */
                                currSet->trgRect.right = x2;

                                /* Add the layer to the overlapping region. */
                                gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                            }
                            else
                            {
                                /* The overlapping region is covered by the opaque layer -
                                    remove the overlapping region. */
                                currSet->trgRect.left = x2;
                            }
                        }
                        else
                        {
                            /*
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             */

                            if (Layer->needPrevious)
                            {
                                /* Create a new target for the bottom non-overlapping region. */
                                gcmONERROR(_CopySet(
                                    Hardware, currSet,
                                    x1, y2,
                                    currSet->trgRect.right, currSet->trgRect.bottom
                                    ));

                                /* Create a new target for the top right non-overlapping region. */
                                gcmONERROR(_CopySet(
                                    Hardware, currSet,
                                    x2, y1,
                                    currSet->trgRect.right, y2
                                    ));

                                /* Set a pointer to the overlapping region. */
                                overlappingSet = currSet;

                                /* Correct coordinates of the top left overlapping region. */
                                currSet->trgRect.right  = x2;
                                currSet->trgRect.bottom = y2;

                                /* Add the layer to the overlapping region. */
                                gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                            }
                            else
                            {
                                /* The overlapping region is covered by the opaque layer -
                                    remove the overlapping region. */

                                /* Create a new target for the bottom non-overlapping region. */
                                gcmONERROR(_CopySet(
                                    Hardware, currSet,
                                    x1, y2,
                                    currSet->trgRect.right, currSet->trgRect.bottom
                                    ));

                                /* Correct coordinates of the top right non-overlapping region. */
                                currSet->trgRect.left   = x2;
                                currSet->trgRect.bottom = y2;
                            }
                        }
                    }
                    else if (y2 == currSet->trgRect.bottom)
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * | X | X |   |
                         * +---+---+---+
                         * | X | X |   |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the bottom right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Create a new target for the bottom left overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the top non-overlapping region. */
                            currSet->trgRect.bottom = y1;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */

                            /* Create a new target for the bottom right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Correct coordinates of the top non-overlapping region. */
                            currSet->trgRect.bottom = y1;
                        }
                    }
                    else
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * | X | X |   |
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y2,
                                currSet->trgRect.right, currSet->trgRect.bottom
                                ));

                            /* Create a new target for the middle right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Create a new target for the middle left overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the top non-overlapping region. */
                            currSet->trgRect.bottom = y1;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */

                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y2,
                                currSet->trgRect.right, currSet->trgRect.bottom
                                ));

                            /* Create a new target for the middle right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Correct coordinates of the top non-overlapping region. */
                            currSet->trgRect.bottom = y1;
                        }
                    }
                }
            }
            else if (x2 == currSet->trgRect.right)
            {
                if (y1 == currSet->trgRect.top)
                {
                    if (y2 == currSet->trgRect.bottom)
                    {
                        /*
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the right overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the left non-overlapping region. */
                            currSet->trgRect.right = x1;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */
                            currSet->trgRect.right = x1;
                        }
                    }
                    else
                    {
                        /*
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                currSet->trgRect.left, y2,
                                x2, currSet->trgRect.bottom
                                ));

                            /* Create a new target for the top right overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the top left non-overlapping region. */
                            currSet->trgRect.right  = x1;
                            currSet->trgRect.bottom = y2;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */

                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                currSet->trgRect.left, y2,
                                x2, currSet->trgRect.bottom
                                ));

                            /* Correct coordinates of the top left non-overlapping region. */
                            currSet->trgRect.right  = x1;
                            currSet->trgRect.bottom = y2;
                        }
                    }
                }
                else if (y2 == currSet->trgRect.bottom)
                {
                    /*
                     * +---+---+---+
                     * |   |   |   |
                     * +---+---+---+
                     * |   | X | X |
                     * +---+---+---+
                     * |   | X | X |
                     * +---+---+---+
                     */

                    if (Layer->needPrevious)
                    {
                        /* Create a new target for the bottom right overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x1, y1,
                            x2, y2
                            ));

                        /* Set a pointer to the overlapping region. */
                        overlappingSet = currSet->next;

                        /* Create a new target for the bottom left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;

                        /* Add the layer to the overlapping region. */
                        gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                    }
                    else
                    {
                        /* The overlapping region is covered by the opaque layer -
                            remove the overlapping region. */

                        /* Create a new target for the bottom left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;
                    }
                }
                else
                {
                    /*
                     * +---+---+---+
                     * |   |   |   |
                     * +---+---+---+
                     * |   | X | X |
                     * +---+---+---+
                     * |   |   |   |
                     * +---+---+---+
                     */

                    if (Layer->needPrevious)
                    {
                        /* Create a new target for the bottom non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y2,
                            x2, currSet->trgRect.bottom
                            ));

                        /* Create a new target for the middle right overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x1, y1,
                            x2, y2
                            ));

                        /* Set a pointer to the overlapping region. */
                        overlappingSet = currSet->next;

                        /* Create a new target for the middle left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;

                        /* Add the layer to the overlapping region. */
                        gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                    }
                    else
                    {
                        /* The overlapping region is covered by the opaque layer -
                            remove the overlapping region. */

                        /* Create a new target for the bottom non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y2,
                            x2, currSet->trgRect.bottom
                            ));

                        /* Create a new target for the middle left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;
                    }
                }
            }
            else
            {
                if (y1 == currSet->trgRect.top)
                {
                    if (y2 == currSet->trgRect.bottom)
                    {
                        /*
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Create a new target for the middle overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the left non-overlapping region. */
                            currSet->trgRect.right = x1;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */

                            /* Create a new target for the right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Correct coordinates of the left non-overlapping region. */
                            currSet->trgRect.right = x1;
                        }
                    }
                    else
                    {
                        /*
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         */

                        if (Layer->needPrevious)
                        {
                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                currSet->trgRect.left, y2,
                                currSet->trgRect.right, currSet->trgRect.bottom
                                ));

                            /* Create a new target for the top right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Create a new target for the middle top overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x1, y1,
                                x2, y2
                                ));

                            /* Set a pointer to the overlapping region. */
                            overlappingSet = currSet->next;

                            /* Correct coordinates of the top left non-overlapping region. */
                            currSet->trgRect.right  = x1;
                            currSet->trgRect.bottom = y2;

                            /* Add the layer to the overlapping region. */
                            gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                        }
                        else
                        {
                            /* The overlapping region is covered by the opaque layer -
                                remove the overlapping region. */

                            /* Create a new target for the bottom non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                currSet->trgRect.left, y2,
                                currSet->trgRect.right, currSet->trgRect.bottom
                                ));

                            /* Create a new target for the top right non-overlapping region. */
                            gcmONERROR(_CopySet(
                                Hardware, currSet,
                                x2, y1,
                                currSet->trgRect.right, y2
                                ));

                            /* Correct coordinates of the top left non-overlapping region. */
                            currSet->trgRect.right  = x1;
                            currSet->trgRect.bottom = y2;
                        }
                    }
                }
                else if (y2 == currSet->trgRect.bottom)
                {
                    /*
                     * +---+---+---+
                     * |   |   |   |
                     * +---+---+---+
                     * |   | X |   |
                     * +---+---+---+
                     * |   | X |   |
                     * +---+---+---+
                     */

                    if (Layer->needPrevious)
                    {
                        /* Create a new target for the bottom right non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x2, y1,
                            currSet->trgRect.right, y2
                            ));

                        /* Create a new target for the bottom middle overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x1, y1,
                            x2, y2
                            ));

                        /* Set a pointer to the overlapping region. */
                        overlappingSet = currSet->next;

                        /* Create a new target for the bottom left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;

                        /* Add the layer to the overlapping region. */
                        gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                    }
                    else
                    {
                        /* The overlapping region is covered by the opaque layer -
                            remove the overlapping region. */

                        /* Create a new target for the bottom right non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x2, y1,
                            currSet->trgRect.right, y2
                            ));

                        /* Create a new target for the bottom left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;
                    }
                }
                else
                {
                    /*
                     * +---+---+---+
                     * |   |   |   |
                     * +---+---+---+
                     * |   | X |   |
                     * +---+---+---+
                     * |   |   |   |
                     * +---+---+---+
                     */

                    if (Layer->needPrevious)
                    {
                        /* Create a new target for the bottom non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y2,
                            currSet->trgRect.right, currSet->trgRect.bottom
                            ));

                        /* Create a new target for the right middle non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x2, y1,
                            currSet->trgRect.right, y2
                            ));

                        /* Create a new target for the middle overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x1, y1,
                            x2, y2
                            ));

                        /* Set a pointer to the overlapping region. */
                        overlappingSet = currSet->next;

                        /* Create a new target for the middle left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;

                        /* Add the layer to the overlapping region. */
                        gcmONERROR(_AppendLayer(Hardware, overlappingSet, Layer));
                    }
                    else
                    {
                        /* Create a new target for the bottom non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y2,
                            currSet->trgRect.right, currSet->trgRect.bottom
                            ));

                        /* Create a new target for the right middle non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            x2, y1,
                            currSet->trgRect.right, y2
                            ));

                        /* Create a new target for the middle left non-overlapping region. */
                        gcmONERROR(_CopySet(
                            Hardware, currSet,
                            currSet->trgRect.left, y1,
                            x1, y2
                            ));

                        /* Correct coordinates of the top non-overlapping region. */
                        currSet->trgRect.bottom = y1;
                    }
                }
            }

            /*******************************************************************
            ** Analyze the overlap: new layer side.
            */

            if (!Layer->needPrevious)
            {
                /* Determine the target rectangle. */
                layerRect = *TargetRect;

                /* Process the part for other overlaps. */
                gcmONERROR(_ProcessLayer(
                    Hardware, Layer, &layerRect, nextSet
                    ));
            }
            else
            {
                if (x1 == TargetRect->left)
                {
                    if (x2 == TargetRect->right)
                    {
                        if (y1 == TargetRect->top)
                        {
                            if (y2 == TargetRect->bottom)
                            {
                                /*
                                 * +---+---+---+
                                 * | X | X | X |
                                 * +---+---+---+
                                 * | X | X | X |
                                 * +---+---+---+
                                 * | X | X | X |
                                 * +---+---+---+
                                 */

                                /* The layer has been covered. */
                            }
                            else
                            {
                                /*
                                 * +---+---+---+
                                 * | X | X | X |
                                 * +---+---+---+
                                 * | X | X | X |
                                 * +---+---+---+
                                 * |   |   |   |
                                 * +---+---+---+
                                 */

                                /*--------------------------------------------------
                                 * Bottom part of the layer has not been covered yet.
                                 */

                                /* Determine the target rectangle. */
                                layerRect.left   = x1;
                                layerRect.top    = y2;
                                layerRect.right  = x2;
                                layerRect.bottom = TargetRect->bottom;

                                /* Process the part for other overlaps. */
                                gcmONERROR(_ProcessLayer(
                                    Hardware, Layer, &layerRect, nextSet
                                    ));
                            }
                        }
                        else if (y2 == TargetRect->bottom)
                        {
                            /*
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Top part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x1;
                            layerRect.top    = TargetRect->top;
                            layerRect.right  = x2;
                            layerRect.bottom = y1;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                        else
                        {
                            /*
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             * | X | X | X |
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Top part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x1;
                            layerRect.top    = TargetRect->top;
                            layerRect.right  = x2;
                            layerRect.bottom = y1;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*------------------------------------------------------
                             * Bottom part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x1;
                            layerRect.top    = y2;
                            layerRect.right  = x2;
                            layerRect.bottom = TargetRect->bottom;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                    }
                    else
                    {
                        if (y1 == TargetRect->top)
                        {
                            if (y2 == TargetRect->bottom)
                            {
                                /*
                                 * +---+---+---+
                                 * | X | X |   |
                                 * +---+---+---+
                                 * | X | X |   |
                                 * +---+---+---+
                                 * | X | X |   |
                                 * +---+---+---+
                                 */

                                /*--------------------------------------------------
                                 * Right part of the layer has not been covered yet.
                                 */

                                /* Determine the target rectangle. */
                                layerRect.left   = x2;
                                layerRect.top    = y1;
                                layerRect.right  = TargetRect->right;
                                layerRect.bottom = y2;

                                /* Process the part for other overlaps. */
                                gcmONERROR(_ProcessLayer(
                                    Hardware, Layer, &layerRect, nextSet
                                    ));
                            }
                            else
                            {
                                /*
                                 * +---+---+---+
                                 * | X | X |   |
                                 * +---+---+---+
                                 * | X | X |   |
                                 * +---+---+---+
                                 * |   |   |   |
                                 * +---+---+---+
                                 */

                                /*--------------------------------------------------
                                 * Top right part of the layer has not been covered yet.
                                 */

                                /* Determine the target rectangle. */
                                layerRect.left   = x2;
                                layerRect.top    = y1;
                                layerRect.right  = TargetRect->right;
                                layerRect.bottom = y2;

                                /* Process the part for other overlaps. */
                                gcmONERROR(_ProcessLayer(
                                    Hardware, Layer, &layerRect, nextSet
                                    ));

                                /*--------------------------------------------------
                                 * Bottom part of the layer has not been covered yet.
                                 */

                                /* Determine the target rectangle. */
                                layerRect.left   = x1;
                                layerRect.top    = y2;
                                layerRect.right  = TargetRect->right;
                                layerRect.bottom = TargetRect->bottom;

                                /* Process the part for other overlaps. */
                                gcmONERROR(_ProcessLayer(
                                    Hardware, Layer, &layerRect, nextSet
                                    ));
                            }
                        }
                        else if (y2 == TargetRect->bottom)
                        {
                            /*
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Top part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x1;
                            layerRect.top    = TargetRect->top;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*-------------------------------------------------------
                             * Bottom right part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x2;
                            layerRect.top    = y1;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                        else
                        {
                            /*
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             * | X | X |   |
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Top part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x1;
                            layerRect.top    = TargetRect->top;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = y1;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*------------------------------------------------------
                             * Middle right part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x2;
                            layerRect.top    = y1;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*------------------------------------------------------
                             * Bottom part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x1;
                            layerRect.top    = y2;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = TargetRect->bottom;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                    }
                }
                else if (x2 == TargetRect->right)
                {
                    if (y1 == TargetRect->top)
                    {
                        if (y2 == TargetRect->bottom)
                        {
                            /*
                             * +---+---+---+
                             * |   | X | X |
                             * +---+---+---+
                             * |   | X | X |
                             * +---+---+---+
                             * |   | X | X |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Left part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = TargetRect->left;
                            layerRect.top    = y1;
                            layerRect.right  = x1;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                        else
                        {
                            /*
                             * +---+---+---+
                             * |   | X | X |
                             * +---+---+---+
                             * |   | X | X |
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Top left part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = TargetRect->left;
                            layerRect.top    = y1;
                            layerRect.right  = x1;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*------------------------------------------------------
                             * Bottom part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = TargetRect->left;
                            layerRect.top    = y2;
                            layerRect.right  = x2;
                            layerRect.bottom = TargetRect->bottom;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                    }
                    else if (y2 == TargetRect->bottom)
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         */

                        /*----------------------------------------------------------
                         * Top part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = TargetRect->top;
                        layerRect.right  = x2;
                        layerRect.bottom = y1;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Bottom left part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = y1;
                        layerRect.right  = x1;
                        layerRect.bottom = y2;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));
                    }
                    else
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * |   | X | X |
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         */

                        /*----------------------------------------------------------
                         * Top part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = TargetRect->top;
                        layerRect.right  = x2;
                        layerRect.bottom = y1;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Middle left part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = y1;
                        layerRect.right  = x1;
                        layerRect.bottom = y2;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Bottom part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = y2;
                        layerRect.right  = x2;
                        layerRect.bottom = TargetRect->bottom;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));
                    }
                }
                else
                {
                    if (y1 == TargetRect->top)
                    {
                        if (y2 == TargetRect->bottom)
                        {
                            /*
                             * +---+---+---+
                             * |   | X |   |
                             * +---+---+---+
                             * |   | X |   |
                             * +---+---+---+
                             * |   | X |   |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Left part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = TargetRect->left;
                            layerRect.top    = y1;
                            layerRect.right  = x1;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*------------------------------------------------------
                             * Right part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x2;
                            layerRect.top    = y1;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                        else
                        {
                            /*
                             * +---+---+---+
                             * |   | X |   |
                             * +---+---+---+
                             * |   | X |   |
                             * +---+---+---+
                             * |   |   |   |
                             * +---+---+---+
                             */

                            /*------------------------------------------------------
                             * Top left part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = TargetRect->left;
                                layerRect.top    = y1;
                            layerRect.right  = x1;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*------------------------------------------------------
                             * Top right part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = x2;
                            layerRect.top    = y1;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = y2;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));

                            /*------------------------------------------------------
                             * Bottom part of the layer has not been covered yet.
                             */

                            /* Determine the target rectangle. */
                            layerRect.left   = TargetRect->left;
                            layerRect.top    = y2;
                            layerRect.right  = TargetRect->right;
                            layerRect.bottom = TargetRect->bottom;

                            /* Process the part for other overlaps. */
                            gcmONERROR(_ProcessLayer(
                                Hardware, Layer, &layerRect, nextSet
                                ));
                        }
                    }
                    else if (y2 == TargetRect->bottom)
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         */

                        /*----------------------------------------------------------
                         * Top part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = TargetRect->top;
                        layerRect.right  = TargetRect->right;
                        layerRect.bottom = y1;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Bottom left part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = y1;
                        layerRect.right  = x1;
                        layerRect.bottom = y2;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Bottom right part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = x2;
                        layerRect.top    = y1;
                        layerRect.right  = TargetRect->right;
                        layerRect.bottom = y2;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));
                    }
                    else
                    {
                        /*
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         * |   | X |   |
                         * +---+---+---+
                         * |   |   |   |
                         * +---+---+---+
                         */

                        /*----------------------------------------------------------
                         * Top part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = TargetRect->top;
                        layerRect.right  = TargetRect->right;
                        layerRect.bottom = y1;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Middle left part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = y1;
                        layerRect.right  = x1;
                        layerRect.bottom = y2;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Middle right part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = x2;
                        layerRect.top    = y1;
                        layerRect.right  = TargetRect->right;
                        layerRect.bottom = y2;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));

                        /*----------------------------------------------------------
                         * Bottom part of the layer has not been covered yet.
                         */

                        /* Determine the target rectangle. */
                        layerRect.left   = TargetRect->left;
                        layerRect.top    = y2;
                        layerRect.right  = TargetRect->right;
                        layerRect.bottom = TargetRect->bottom;

                        /* Process the part for other overlaps. */
                        gcmONERROR(_ProcessLayer(
                            Hardware, Layer, &layerRect, nextSet
                            ));
                    }
                }
            }

            /* Done processing the layer. */
            goto OnError;
        }

        /***********************************************************************
        ** No overlap found - add the layer to the tail.
        */

        /* Allocate new set. */
        gcmONERROR(gcsCONTAINER_AllocateRecord(
            &Hardware->composition.freeSets,
            (gctPOINTER *) &tempSet
            ));

        /* Add to the list. */
        tempSet->prev =  Hardware->composition.head.prev;
        tempSet->next = &Hardware->composition.head;
        Hardware->composition.head.prev->next = tempSet;
        Hardware->composition.head.prev       = tempSet;

        /* Initialize the set. */
        tempSet->blur     = gcvFALSE;
        tempSet->trgRect  = *TargetRect;
        tempSet->nodeHead =
        tempSet->nodeTail = gcvNULL;

        /* Allocate new layer node. */
        gcmONERROR(gcsCONTAINER_AllocateRecord(
            &Hardware->composition.freeNodes,
            (gctPOINTER *) &tempNode
            ));

        /* Add to the list. */
        tempSet->nodeHead =
        tempSet->nodeTail = tempNode;

        /* Initialize the node. */
        tempNode->layer = Layer;
        tempNode->next  = gcvNULL;
    }

OnError:
    return status;
}

static gceSTATUS
_FreeTemporarySurface(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Synchronized
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsSURF_INFO_PTR tempBuffer;

    gcmHEADER_ARG("Hardware=0x%08X Synchronized=%d", Hardware, Synchronized);

    /* Surface shortcut. */
    tempBuffer = &Hardware->composition.tempBuffer;

    /* Is there a surface allocated? */
    if (tempBuffer->node.pool != gcvPOOL_UNKNOWN)
    {
        /* Unlock the node. */
        gcmONERROR(gcoHARDWARE_Unlock(
            &tempBuffer->node, tempBuffer->type
            ));

        /* Schedule deletion. */
        if (Synchronized)
        {
            gcmONERROR(gcoHARDWARE_ScheduleVideoMemory(
                &tempBuffer->node
                ));
        }

        /* Not synchronized --> delete immediately. */
        else
        {
            gcsHAL_INTERFACE iface;

            /* Free the video memory. */
            iface.command = gcvHAL_FREE_VIDEO_MEMORY;
            iface.u.FreeVideoMemory.node = tempBuffer->node.u.normal.node;

            /* Call kernel service. */
            gcmONERROR(gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(gcsHAL_INTERFACE),
                &iface, gcmSIZEOF(gcsHAL_INTERFACE)
                ));
        }

        /* Reset the temporary surface. */
        gcmONERROR(gcoOS_ZeroMemory(tempBuffer, sizeof(gcsSURF_INFO)));
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_AllocateTemporarySurface(
    IN gcoHARDWARE Hardware,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gceSURF_FORMAT Format,
    IN gceSURF_TYPE Type
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
    gcsSURF_INFO_PTR tempBuffer;
    gcsCOMPOSITION_LAYER_PTR tempLayer;
    gcsSURF_FORMAT_INFO_PTR format[2];

    gcmHEADER_ARG("Hardware=0x%08X Width=%d Height=%d Format=%d Type=%d",
                  Hardware, Width, Height, Format, Type);

    /* Surface shortcut. */
    tempBuffer = &Hardware->composition.tempBuffer;

    /* Do we have a compatible surface? */
    if ((tempBuffer->type == Type) &&
        (tempBuffer->format == Format) &&
        (tempBuffer->rect.right >= (gctINT) Width) &&
        (tempBuffer->rect.bottom >= (gctINT) Height))
    {
        status = gcvSTATUS_OK;
    }
    else
    {
        /* Delete existing buffer. */
        gcmONERROR(_FreeTemporarySurface(Hardware, gcvTRUE));

        tempBuffer->alignedWidth  = Width;
        tempBuffer->alignedHeight = Height;

        /* Locate format record. */
        gcmONERROR(gcoSURF_QueryFormat(Format, format));

        /* Align the width and height. */
        gcmONERROR(gcoHARDWARE_AlignToTile(
            Type,
            Format,
            &tempBuffer->alignedWidth,
            &tempBuffer->alignedHeight,
            gcvNULL
            ));

        /* Init the interface structure. */
        iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        iface.u.AllocateLinearVideoMemory.bytes     = tempBuffer->alignedWidth
                                                    * format[0]->bitsPerPixel / 8
                                                    * tempBuffer->alignedHeight;
        iface.u.AllocateLinearVideoMemory.alignment = 64;
        iface.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;
        iface.u.AllocateLinearVideoMemory.type      = Type;

        /* Call kernel service. */
        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL, IOCTL_GCHAL_INTERFACE,
            &iface, sizeof(gcsHAL_INTERFACE),
            &iface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmONERROR(iface.status);

        /* Set the new parameters. */
        tempBuffer->type                = Type;
        tempBuffer->format              = Format;
        tempBuffer->stride              = tempBuffer->alignedWidth
                                        * format[0]->bitsPerPixel / 8;
        tempBuffer->size                = iface.u.AllocateLinearVideoMemory.bytes;
        tempBuffer->node.valid          = gcvFALSE;
        tempBuffer->node.lockCount      = 0;
        tempBuffer->node.lockedInKernel = gcvFALSE;
        tempBuffer->node.logical        = gcvNULL;
        tempBuffer->node.physical       = ~0U;

        tempBuffer->node.pool               = iface.u.AllocateLinearVideoMemory.pool;
        tempBuffer->node.u.normal.node      = iface.u.AllocateLinearVideoMemory.node;
        tempBuffer->node.u.normal.cacheable = gcvFALSE;

        tempBuffer->samples.x = 1;
        tempBuffer->samples.y = 1;

        tempBuffer->rect.left   = 0;
        tempBuffer->rect.top    = 0;
        tempBuffer->rect.right  = Width;
        tempBuffer->rect.bottom = Height;

        /* Lock it. */
        gcmONERROR(gcoHARDWARE_Lock(
            &tempBuffer->node, gcvNULL, gcvNULL
            ));
    }

    /* Layer shortcut. */
    tempLayer = &Hardware->composition.tempLayer;

    /* Initialize the target layer. */
    tempLayer->input.operation = gcvCOMPOSE_LAYER;

    tempLayer->surface  = tempBuffer;
    tempLayer->type     = tempBuffer->type;
    tempLayer->stride   = tempBuffer->stride;

    tempLayer->input.srcRect.left   = 0;
    tempLayer->input.srcRect.top    = 0;
    tempLayer->input.srcRect.right  = Width;
    tempLayer->input.srcRect.bottom = Height;

    tempLayer->input.trgRect.left   = 0;
    tempLayer->input.trgRect.top    = 0;
    tempLayer->input.trgRect.right  = Width;
    tempLayer->input.trgRect.bottom = Height;

    tempLayer->sizeX    = Width;
    tempLayer->sizeY    = Height;

    tempLayer->samplesX = 1;
    tempLayer->samplesY = 1;

    tempLayer->input.alphaValue     = 0xFF;
    tempLayer->input.premultiplied  = gcvFALSE;
    tempLayer->input.enableBlending = gcvFALSE;

    tempLayer->replaceAlpha  = gcvFALSE;
    tempLayer->modulateAlpha = gcvFALSE;

    tempLayer->flip = gcvFALSE;

    gcmONERROR(_TranslateSourceFormat(
        Format, &tempLayer->format, &tempLayer->hasAlpha
        ));

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_InitResources(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources
    )
{
    /* Reset the current sampler. */
    Resources->currSampler = 0;

    /* Reset temporary register indices. */
    Resources->prevTemp = 0;
    Resources->currTemp = 1;

    /* Reset the current shader program counter. */
    Resources->currPC = 0;

    /* Compute the instruction state address. */
    Resources->currPCaddress

        /* Hardware register address of the first instruction. */
        = Hardware->psInstBase

        /* Composition base. */
        + Hardware->composition.instBase * 4;

    /* Compute the constant state base address. */
    Resources->constStateBase

        /* Hardware register address of the first constant. */
        = Hardware->psConstBase

        /* Composition base. */
        + Hardware->composition.constBase * 4;

    /* Determine the access offset for the shader. */
    Resources->constOffset = Hardware->unifiedConst
        ? 0
        : Hardware->composition.constBase;

    /* Reset the current constant indices. Clear color constant registers
       are allocated from the base towards the higher addresses and the
       color multipliers are allocated in reverse from the end towards
       the lower addresses.*/
    Resources->nextClearValue = 0;
    Resources->nextAlphaValue = Hardware->composition.maxConstCount * 4 - 1;

    return gcvSTATUS_OK;
}

static gceSTATUS
_AllocateBuffer(
    IN gcsCOMPOSITION_STATE_BUFFER_PTR Buffer
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    /***********************************************************************
    ** Create the state buffer synchronization signal.
    */

    if (Buffer->signal == gcvNULL)
    {
        gcmONERROR(gcoOS_CreateSignal(
            gcvNULL, gcvTRUE, &Buffer->signal
            ));

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_SIGNAL,
            "%s(%d): composition signal created 0x%08X",
            __FUNCTION__, __LINE__,
            Buffer->signal
            );

        gcmONERROR(gcoOS_Signal(gcvNULL, Buffer->signal, gcvTRUE));
    }

    /***********************************************************************
    ** Allocate the state buffer.
    */

    /* Allocate the physical buffer for the command. */
    iface.command = gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY;
    iface.u.AllocateContiguousMemory.bytes = gcdCOMP_BUFFER_SIZE;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL, IOCTL_GCHAL_INTERFACE,
        &iface, sizeof(iface),
        &iface, sizeof(iface)
        ));

    /* Set the buffer address. */
    Buffer->size      = iface.u.AllocateContiguousMemory.bytes;
    Buffer->physical  = iface.u.AllocateContiguousMemory.physical;
    Buffer->address   = iface.u.AllocateContiguousMemory.address;
    Buffer->logical   = (gctUINT32_PTR) iface.u.AllocateContiguousMemory.logical;

    /* Initialize the current buffer (reserve space for the contol state). */
    Buffer->head      = Buffer->logical;
    Buffer->tail      = Buffer->logical + 2;
    Buffer->available = Buffer->size - Buffer->reserve;
    Buffer->count     = 0;
    Buffer->rectangle = Buffer->tail;

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_FinalizeBuffer(
    IN gcoHARDWARE Hardware,
    IN gcsCOMPOSITION_STATE_BUFFER_PTR Buffer
    )
{
    gceSTATUS status;
    gctUINT unaligned, aligned;
    gctUINT executeSize;
    gctINT padding;
    gctUINT offset = 0;
    gcsHAL_INTERFACE iface;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    /* Compute the unaligned size. */
    unaligned
        = (gctUINT8_PTR) Buffer->tail
        - (gctUINT8_PTR) Buffer->head;

    /* Reserve space for EVENT. */
    if (!Hardware->composition.synchronous)
    {
        offset     = unaligned;
        unaligned += Hardware->composition.eventSize;
    }

    /* Hardware requires 64-byte alignment. Per hardware requirement,
       make sure to add an extra 64 bytes of alignment in case if already
       64-byte aligned. */
    aligned = gcmALIGN((unaligned + 1), 64);

    /* Total size less the control state. */
    executeSize = aligned - gcmSIZEOF(gctUINT32) * 2;

    /* Pad with dummy states. */
    padding = aligned - unaligned;

    while (padding > 0)
    {
        /* The buffer size is 64-byte aligned and we always execute in 64-byte
           aligned chunks, therefore we should always have enough space for padding. */
        if (Buffer->available == 0)
        {
            gcmONERROR(gcvSTATUS_OUT_OF_RESOURCES);
        }

        gcmiCOMPOSITIONSTATE(Buffer, 0x0000FFFF, 0x00000000);
        padding -= 8;
    }

    gcmASSERT(padding == 0);

    /* Program the size of the state buffer. */
    Buffer->head[0] = 0xFFFFFFFF;
    Buffer->head[1] = executeSize >> 3;

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): unaligned = %d, aligned = %d\n",
        __FUNCTION__, __LINE__,
        unaligned, aligned
        );

    if (Hardware->composition.synchronous)
    {
        /* Compute the offset from beginning of the buffer. */
        offset
            = (gctUINT8_PTR) Buffer->head
            - (gctUINT8_PTR) Buffer->logical;
        gcmASSERT((offset & 63) == 0);

        /* Determine the size of the buffer to reserve. */
        reserveSize

            /* A batch of two states. */
            = gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0C02, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0C02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            /* Set buffer location. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0C02,
                Buffer->address + offset
                );

            /* Start loading. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0C03,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);
    }
    else
    {
        /* Submit the buffer. */
        iface.command = gcvHAL_COMPOSE;
        iface.u.Compose.physical    = Buffer->physical;
        iface.u.Compose.logical     = Buffer->logical;
        iface.u.Compose.offset      = offset;
        iface.u.Compose.size        = executeSize;
        iface.u.Compose.signal      = Buffer->signal;
        iface.u.Compose.process     = gcoOS_GetCurrentProcessID();
        iface.u.Compose.userProcess = Hardware->composition.process;
        iface.u.Compose.userSignal1 = Hardware->composition.signal1;
        iface.u.Compose.userSignal2 = Hardware->composition.signal2;

        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL, IOCTL_GCHAL_INTERFACE,
            &iface, sizeof(iface),
            &iface, sizeof(iface)
            ));
    }

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_ExecuteBuffer(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gcsHAL_INTERFACE iface;

    /* Assume the current buffer. */
    buffer = Hardware->composition.compStateBufferCurrent;

    /* Prepare the buffer for execution. */
    gcmONERROR(_FinalizeBuffer(Hardware, buffer));

    if (Hardware->composition.synchronous)
    {
        /* If this is the last composition pass, schedule user events. */
        if (Hardware->composition.signal1 != gcvNULL)
        {
            iface.command            = gcvHAL_SIGNAL;
            iface.u.Signal.signal    = Hardware->composition.signal1;
            iface.u.Signal.auxSignal = gcvNULL;
            iface.u.Signal.process   = Hardware->composition.process;
            iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

            gcmONERROR(gcoHARDWARE_CallEvent(&iface));
        }

        if (Hardware->composition.signal2 != gcvNULL)
        {
            iface.command            = gcvHAL_SIGNAL;
            iface.u.Signal.signal    = Hardware->composition.signal2;
            iface.u.Signal.auxSignal = gcvNULL;
            iface.u.Signal.process   = Hardware->composition.process;
            iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

            gcmONERROR(gcoHARDWARE_CallEvent(&iface));
        }

        /* Execute the buffer. */
        gcmONERROR(gcoHARDWARE_Commit());

        /* Update buffer pointers. */
        buffer->head       = buffer->tail;
        buffer->tail      += 2;
        buffer->available -= gcmSIZEOF(gctUINT32) * 2;
        buffer->count      = 0;
        buffer->rectangle  = buffer->tail;
    }

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_ProbeBuffer(
    IN gcoHARDWARE Hardware,
    IN gctINT Size,
    OUT gcsCOMPOSITION_STATE_BUFFER_PTR * Buffer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gcsHAL_INTERFACE iface;

    /* Hardware requires an extra 64 bytes of alignment
       at the end of the buffer. */
    Size += 64;

    /* Shortcut to the current buffer. */
    buffer = Hardware->composition.compStateBufferCurrent;

    /* The buffer has not been created yet? */
    if (buffer->size == 0)
    {
        gcmONERROR(_AllocateBuffer(buffer));
    }

    /* Run out of space in the existing buffer? */
    else if (buffer->available < Size)
    {
        /* Point to next buffer . */
        gcsCOMPOSITION_STATE_BUFFER_PTR next = buffer->next;

        /* Last incompleted rectangle. */
        gctUINT32_PTR start = buffer->rectangle;
        gctUINT       count = (buffer->tail - buffer->rectangle) / 2;

        /* We should never run out in asynchronous mode. */
        if (!Hardware->composition.synchronous)
        {
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        /* The next buffer has not been created yet? */
        if (next->size == 0)
        {
            gcmONERROR(_AllocateBuffer(next));
        }

        /* Wait until the next buffer is ready. */
        else
        {
            gcmONERROR(gcoOS_WaitSignal(gcvNULL, next->signal, gcvINFINITE));

            /* Initialize the current buffer (reserve space for the contol state). */
            next->head      = next->logical;
            next->tail      = next->logical + 2;
            next->available = next->size - buffer->reserve;
            next->count     = 0;
            next->rectangle = next->tail;
        }

        /* Since there's no context buffer for composition, we need make sure
           all states for a rectangle are uploaded together.
           When state buffer runs out, there could be an INCOMPLETED rectangle.
           We need copy states of the incompleted rectangle to next buffer. */
        if (count > 0)
        {
            gcoOS_MemCopy(next->tail, start, count * 2 * gcmSIZEOF(gctUINT32));

            next->tail      += count * 2;
            next->available -= count * 2 * gcmSIZEOF(gctUINT32);
            next->count      = count;
        }

        /* Execute accumulated rectangles if any. */
        if (buffer->count && buffer->rectangle != buffer->tail)
        {
            /* Do not load the incompleted rect. */
            buffer->tail   = buffer->rectangle;

            gcmONERROR(_FinalizeBuffer(Hardware, buffer));
        }

        /* Reset the signal. */
        gcmONERROR(gcoOS_Signal(gcvNULL, buffer->signal, gcvFALSE));

        /* Schedule an event for the current buffer. */
        iface.command            = gcvHAL_SIGNAL;
        iface.u.Signal.signal    = buffer->signal;
        iface.u.Signal.auxSignal = gcvNULL;
        iface.u.Signal.process   = Hardware->composition.process;
        iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

        gcmONERROR(gcoHARDWARE_CallEvent(&iface));
        gcmONERROR(gcoHARDWARE_Commit());

        /* Switch to the next buffer. */
        Hardware->composition.compStateBufferCurrent = buffer = next;
    }

    /* We have to have enough room in the buffer at this point. */
    if (buffer->available < Size)
    {
        gcmONERROR(gcvSTATUS_OUT_OF_RESOURCES);
    }

    * Buffer = buffer;

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_SetSyncronousMode(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;

    /* Already in synchronous mode? */
    if (Hardware->composition.synchronous)
    {
        status = gcvSTATUS_OK;
    }
    else
    {
        /* Shortcut to the current buffer. */
        buffer = Hardware->composition.compStateBufferCurrent;

        /* The buffer has not been created yet? */
        if (buffer->size == 0)
        {
            Hardware->composition.synchronous = gcvTRUE;
            status = gcvSTATUS_OK;
        }
        else
        {
            /* Make sure the composition is done. */
            gcmONERROR(gcoOS_WaitSignal(gcvNULL, buffer->signal, gcvINFINITE));

            /* Initialize the current buffer (reserve space for the contol state). */
            buffer->head      = buffer->logical;
            buffer->tail      = buffer->logical + 2;
            buffer->available = buffer->size - buffer->reserve;
            buffer->count     = 0;
            buffer->rectangle = buffer->tail;

            /* Change mode to synchronous. */
            Hardware->composition.synchronous = gcvTRUE;
        }
    }

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_SetAsyncronousMode(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    /* Shortcut to the current buffer. */
    buffer = Hardware->composition.compStateBufferCurrent;

    /* Already in asynchronous mode? */
    if (!Hardware->composition.synchronous)
    {
        /* The buffer has not been created yet? */
        if (buffer->size == 0)
        {
            status = gcvSTATUS_OK;
        }
        else
        {
            /* Make sure the previous composition is done. */
            gcmONERROR(gcoOS_WaitSignal(gcvNULL, buffer->signal, gcvINFINITE));
        }
    }
    else
    {
        /* The buffer has not been created yet? */
        if (buffer->size == 0)
        {
            Hardware->composition.synchronous = gcvFALSE;
            status = gcvSTATUS_OK;
        }
        else
        {
            /* Determine the size of the buffer to reserve. */
            reserveSize

                /* A batch of two states. */
                = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

            /* Reserve space in the command buffer. */
            gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

            /* Set buffer location. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0C02, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0C02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0C02, buffer->address );     gcmENDSTATEBATCH(reserve, memory);};

            /* Validate the state buffer. */
            gcmENDSTATEBUFFER(reserve, memory, reserveSize);

            /* Execute the buffer and wait for completion. */
            gcmONERROR(gcoHARDWARE_Commit());

            gcmONERROR(gcoHARDWARE_Stall());

            /* Make sure the composition is done. */
            gcmONERROR(gcoOS_Signal(gcvNULL, buffer->signal, gcvTRUE));

            /* Change mode to synchronous. */
            Hardware->composition.synchronous = gcvFALSE;
        }
    }

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_TriggerComposition(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources
    )
{
    gceSTATUS status;
    gctUINT requiredSize;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT startPC, endPC;
    gctUINT attributeCount;
    gctUINT inputCount;

    /* Determine the required buffer size. */
    requiredSize = (gcmSIZEOF(gctUINT32) * 2) * 3;

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    /* Determine start and end PCs. */
    startPC = Hardware->composition.instBase;
    endPC   = Hardware->composition.instBase + Resources->currPC;

    /* Determine the counts. */
    attributeCount = Resources->currSampler;
    inputCount     = attributeCount + 1;

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C04,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (startPC) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (endPC) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): startPC = %d, endPC = %d\n",
        __FUNCTION__, __LINE__,
        startPC, endPC
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C06,

        /* Number of valid attributes for the rectangle, each attrib has X and Y coords. */
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (attributeCount) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))

        /* Temp register in shader that holds output color. */
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp) & ((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)))

        /* Number of temp registers used in shader. */
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)))

        /* Number of cycles to transfer between RA and PS. */
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (inputCount) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): attributeCount = %d, result register = %d,"
               " temp count = %d, input count = %d\n",
        __FUNCTION__, __LINE__,
        attributeCount,
        Resources->prevTemp,
        Resources->currTemp,
        inputCount
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C03,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
        );

    /* Tag rectagnle start. */
    buffer->rectangle = buffer->tail;

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_InitCompositionState(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT requiredSize;

    if (!Hardware->composition.initDone)
    {
        /* Determine the required buffer size. */
        requiredSize = Hardware->unifiedConst
            ? (gcmSIZEOF(gctUINT32) * 2) * 3
            : (gcmSIZEOF(gctUINT32) * 2) * 2;

        /* Make sure there is enough space in the buffer. */
        gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C10,
            0xFFFFFFFF
            );

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C11,
            0xFFFFFFFF
            );

        if (Hardware->unifiedConst)
        {
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                "%s(%d): Unified constant base: %d\n",
                __FUNCTION__, __LINE__,
                Hardware->composition.constBase
                );

            gcmiCOMPOSITIONSTATE(
                buffer,
                0x0C12,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (Hardware->composition.constBase) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
                );
        }
        else
        {
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                "%s(%d): Not using unified constants.\n",
                __FUNCTION__, __LINE__
                );
        }

        Hardware->composition.initDone = gcvTRUE;
    }

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_SetCompositionTarget(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsSURF_INFO_PTR Target
    )
{
    static gctUINT32 _tiling[] =
    {
        0x2, /* gcvLINEAR */
        0x0, /* gcvTILED */
        0x1, /* gcvSUPERTILED */
        ~0U, /* gcvMULTI_TILED */
        ~0U                                 /* gcvMULTI_SUPERTILED */
    };

    gceSTATUS status = gcvSTATUS_OK;

    gctUINT32 bankOffsetBytes = 0;

    gctUINT32 tiling;
    gctUINT32 sampling;
    gctUINT samples;
    gctUINT32 format;
    gctUINT32 swizzle;
    gctBOOL hasAlpha;

    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT requiredSize;

    gcmHEADER_ARG(
        "Hardware=0x%08X Resources=0x%08X Target=0x%08X",
        Hardware, Resources, Target
        );

    /* Already set? */
    if (Hardware->composition.target == Target)
    {
        goto OnError;
    }

    /* Determine the required buffer size. */
    requiredSize
        = gcmSIZEOF(gctUINT32) * 2      /* 0x0C05 */
        + gcmSIZEOF(gctUINT32) * 2      /* 0x0C07 */
        + gcmSIZEOF(gctUINT32) * 2;     /* 0x0C08 */

    if ((Target == Hardware->colorStates.surface) && !Target->tileStatusDisabled)
    {
        requiredSize += gcmSIZEOF(gctUINT32) * 2;
    }

    if (Hardware->pixelPipes > 1)
    {
        requiredSize += gcmSIZEOF(gctUINT32) * 2;
    }

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    /* Is this the current target surface? */
    if ((Target == Hardware->colorStates.surface) &&
        !Target->tileStatusDisabled)
    {
        /* Mark as disabled. */
        Target->tileStatusDisabled = gcvTRUE;

        /* Disable color tile status. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)));

        /* Make sure auto-disable is turned off as well. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)));

        /* Make sure compression is turned off as well. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)));

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0595,
            Hardware->memoryConfig
            );

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): The target is the current render target.\n",
            __FUNCTION__, __LINE__
            );
    }

    /* Convert the format. */
    gcmONERROR(_TranslateTargetFormat(
        Hardware, Target->format, &format, &swizzle, &hasAlpha
        ));

    /* Determine tiling. */
    if (Target->tiling >= gcmCOUNTOF(_tiling))
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    tiling = _tiling[Target->tiling];

    if (tiling == ~0U)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Determine multisampling. */
    samples = Target->samples.x * Target->samples.y;

    switch (samples)
    {
    case 1:
        sampling = 0x0;
        break;

    case 2:
        sampling = 0x1;
        break;

    case 4:
        sampling = 0x2;
        break;

    default:
        sampling = ~0U;
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /***************************************************************************
    ** Set destination buffer.
    */

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): stride=%d tiling=%d format=%d swizzle=0x%04X sampling=%d\n",
        __FUNCTION__, __LINE__,
        Target->stride, tiling, format, swizzle, sampling
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C05,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:0) - (0 ? 16:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:0) - (0 ? 16:0) + 1))))))) << (0 ? 16:0))) | (((gctUINT32) ((gctUINT32) (Target->stride) & ((gctUINT32) ((((1 ? 16:0) - (0 ? 16:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:0) - (0 ? 16:0) + 1))))))) << (0 ? 16:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (tiling) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:24) - (0 ? 28:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:24) - (0 ? 28:24) + 1))))))) << (0 ? 28:24))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 28:24) - (0 ? 28:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:24) - (0 ? 28:24) + 1))))))) << (0 ? 28:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (sampling) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22)))
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C07,
        swizzle
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): trgPhysical1=0x%08X\n",
        __FUNCTION__, __LINE__,
        Target->node.physical
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C08,
        Target->node.physical
        );

    if (Hardware->pixelPipes > 1)
    {
        if (Target->type == gcvSURF_RENDER_TARGET)
        {
            gctINT height, half;

            height = gcmALIGN(
                Target->alignedHeight / 2, Target->superTiled ? 64 : 4
                );

            half = height * Target->stride;

            /* Extra pages needed to offset sub-buffers to different banks. */
#if gcdENABLE_BANK_ALIGNMENT
            gcmONERROR(gcoSURF_GetBankOffsetBytes(
                gcvNULL,
                Target->type, half, &bankOffsetBytes
                ));

            /* If surface doesn't have enough padding, then don't offset it. */
            if (Target->size <
                ((Target->alignedHeight * Target->stride) + bankOffsetBytes))
            {
                bankOffsetBytes = 0;
            }
#endif
            half += bankOffsetBytes;

            gcmiCOMPOSITIONSTATE(
                buffer,
                0x0C08 + 1,
                Target->node.physical + half
                );

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                "%s(%d): trgPhysical2=0x%08X (half)\n",
                __FUNCTION__, __LINE__,
                Target->node.physical + half
                );
        }
        else
        {
            gcmiCOMPOSITIONSTATE(
                buffer,
                0x0C08 + 1,
                Target->node.physical
                );

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                "%s(%d): trgPhysical2=0x%08X\n",
                __FUNCTION__, __LINE__,
                Target->node.physical
                );
        }
    }

    /* Set current target. */
    Hardware->composition.target = Target;

OnError:
    gcmFOOTER();

    /* Return the status. */
    return status;
}

static gceSTATUS
_ResetConstantAllocator(
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources
    )
{
    /* Initialize constant allocator. */
    Resources->constAllocator = 0;
    return gcvSTATUS_OK;
}

static gceSTATUS
_AllocateConstant(
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsiCONST_ALLOCATOR_PTR Constant,
    IN gctUINT Alignment
    )
{
    Resources->constAllocator
        = gcmALIGN(Resources->constAllocator, Alignment);

    Constant->index = Resources->constAllocator++;

    Constant->address = Resources->constOffset + Constant->index / 4;
    Constant->swizzle = _compSwizzle[Constant->index % 4];

    return gcvSTATUS_OK;
}

static gceSTATUS
_AllocateClearValue(
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_LAYER_PTR Layer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT nextClearValue;

    gcmHEADER_ARG(
        "Resources=0x%08X Layer=0x%08X",
        Resources, Layer
        );

    /* Determine the next clear color constant register. */
    nextClearValue = Resources->nextClearValue + 4;

    /* Out of registers? */
    if (nextClearValue > Resources->nextAlphaValue)
    {
        gcmONERROR(gcvSTATUS_OUT_OF_RESOURCES);
    }

    /* Assign clear color register. */
    Layer->constRegister = Resources->nextClearValue;

    /* Allocate the register. */
    Resources->nextClearValue = nextClearValue;

OnError:
    gcmFOOTER();

    /* Return the status. */
    return status;
}

static gceSTATUS
_AllocateAlphaValue(
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_LAYER_PTR Layer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT nextAlphaValue;

    gcmHEADER_ARG(
        "Resources=0x%08X Layer=0x%08X",
        Resources, Layer
        );

    /* Determine the next clear color constant register. */
    nextAlphaValue = Resources->nextAlphaValue - 1;

    /* Out of registers? */
    if (nextAlphaValue < Resources->nextClearValue)
    {
        gcmONERROR(gcvSTATUS_OUT_OF_RESOURCES);
    }

    /* Assign alpha value register. */
    Layer->constRegister = Resources->nextAlphaValue;

    /* Allocate the register. */
    Resources->nextAlphaValue = nextAlphaValue;

OnError:
    gcmFOOTER();

    /* Return the status. */
    return status;
}

static gceSTATUS
_SetCompositionSampler(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_LAYER_PTR Layer,
    IN gcsRECT_PTR SrcWindow,
    IN gcsRECT_PTR TrgWindow,
    IN gcsRECT_PTR TrgEffective
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gctUINT32 bankOffsetBytes = 0;

    gctFLOAT texLeft   = 0.0f;
    gctFLOAT texRight  = 1.0f;
    gctFLOAT texTop    = 0.0f;
    gctFLOAT texBottom = 1.0f;

    gcuFLOAT_UINT32 attrX, attrY;
    gcuFLOAT_UINT32 attrXDX, attrXDY;
    gcuFLOAT_UINT32 attrYDX, attrYDY;

    gctFLOAT srcEffLeft, srcEffTop;
    gctFLOAT srcEffRight, srcEffBottom;
    gctBOOL hAlignmentAvailable;

    gctUINT32 srcAlignment  = 0;
    gctUINT32 srcAddressing = 0;
    gctINT srcSizeX, srcSizeY;
    gctUINT32 srcTileStatus1;
    gctUINT32 srcTileStatus2;
    gctUINT32 srcTileFormat;

    gctUINT sampler;

    gcsCOMPOSITION_STATE_BUFFER_PTR buffer = gcvNULL;
    gctUINT requiredSize;

    gcmHEADER_ARG(
        "Hardware=0x%08X Resources=0x%08X Layer=0x%08X TrgEffective=0x%08X",
        Hardware, Resources, Layer, TrgEffective
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): hasAlpha=%d enableBlending=%d premultiplied=%d alphaValue=%d",
        __FUNCTION__, __LINE__,
        Layer->hasAlpha, Layer->input.enableBlending,
        Layer->input.premultiplied, Layer->input.alphaValue
        );

    /***********************************************************************
    ** Ensure buffer space.
    */

    /* Determine the required buffer size. */
    if (Layer->input.operation == gcvCOMPOSE_CLEAR)
    {
        requiredSize
            = (gcmSIZEOF(gctUINT32) * 2) * 4;           /* Constant values. */
    }

    else if (Layer->input.operation == gcvCOMPOSE_LAYER)
    {
        requiredSize
            = (gcmSIZEOF(gctUINT32) * 2) * 6            /* Attribute states. */
            + (gcmSIZEOF(gctUINT32) * 2) * 4            /* Source configuration states. */
            +  gcmSIZEOF(gctUINT32) * 2                 /* 0x0C80 */
            +  gcmSIZEOF(gctUINT32) * 2;                /* 0x0C68 */

        if (Hardware->pixelPipes > 1)
        {
            requiredSize
                += gcmSIZEOF(gctUINT32) * 2;
        }

        if (Layer->surface->tileStatusNode.valid && !Layer->surface->tileStatusDisabled)
        {
            requiredSize
                += (gcmSIZEOF(gctUINT32) * 2) * 2;
        }
    }
    else
    {
        requiredSize = 0;
    }

    if (Layer->modulateAlpha || Layer->replaceAlpha)
    {
        requiredSize
            += gcmSIZEOF(gctUINT32) * 2;
    }

    if (requiredSize > 0)
    {
        /* Make sure there is enough space in the buffer. */
        gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));
    }

    /***********************************************************************
    ** Set the constants.
    */

    /* Are we in clear? */
    if (Layer->input.operation == gcvCOMPOSE_CLEAR)
    {
        gctUINT constAddress;

        /* Allocate a constant register for the clear color. */
        gcmONERROR(_AllocateClearValue(Resources, Layer));

        /* Determine the register address. */
        constAddress = Resources->constStateBase + Layer->constRegister;

        /* Load the clear value into the constant registers. */
        gcmiCOMPOSITIONSTATE(
            buffer, constAddress + 0, * (gctUINT32_PTR) (&Layer->input.r)
            );

        gcmiCOMPOSITIONSTATE(
            buffer, constAddress + 1, * (gctUINT32_PTR) (&Layer->input.g)
            );

        gcmiCOMPOSITIONSTATE(
            buffer, constAddress + 2, * (gctUINT32_PTR) (&Layer->input.b)
            );

        gcmiCOMPOSITIONSTATE(
            buffer, constAddress + 3, * (gctUINT32_PTR) (&Layer->input.a)
            );

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): setting clear color %.1f,%.1f,%.1f,%.1f to %d (0x%04X).\n",
            __FUNCTION__, __LINE__,
            Layer->input.r, Layer->input.g, Layer->input.b, Layer->input.a,
            Layer->constRegister, constAddress
            );
    }

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): modulateAlpha=%d replaceAlpha=%d.\n",
        __FUNCTION__, __LINE__,
        Layer->modulateAlpha, Layer->replaceAlpha
        );

    if (Layer->modulateAlpha || Layer->replaceAlpha)
    {
        gctUINT constAddress;
        gctFLOAT constValue;

        /* Allocate a constant register for the alpha value. */
        gcmONERROR(_AllocateAlphaValue(Resources, Layer));

        /* Determine the register address. */
        constAddress = Resources->constStateBase + Layer->constRegister;

        /* Convert the alpha value. */
        constValue = Layer->input.alphaValue / 255.0f;

        /* Load the alpha value into the constant register. */
        gcmiCOMPOSITIONSTATE(
            buffer, constAddress, * (gctUINT32_PTR) (&constValue)
            );

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): setting alpha value %.1f to %d (0x%04X).\n",
            __FUNCTION__, __LINE__,
            Layer->input.alphaValue,
            Layer->constRegister, constAddress
            );
    }

    /* Allocate a sampler. */
    sampler = Layer->samplerNumber = Resources->currSampler;
    Resources->currSampler += 1;

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): using sampler %d.\n",
        __FUNCTION__, __LINE__,
        sampler
        );

    /* Is this a regular layer? */
    if (Layer->input.operation == gcvCOMPOSE_LAYER)
    {
       gctFLOAT area;
       gctFLOAT dx0, dx1, dy0, dy1;

       gctFLOAT trgX0, trgY0;
       gctFLOAT trgX1, trgY1;
       gctFLOAT trgX2, trgY2;

       gctFLOAT a0x, a0y;
       gctFLOAT a1x, a1y;
       gctFLOAT a2x, a2y;

        /***********************************************************************
        ** Compute rectangle attributes.
        */
        srcEffLeft   = (gctFLOAT) SrcWindow->left;
        srcEffRight  = (gctFLOAT) SrcWindow->right;
        srcEffTop    = (gctFLOAT) SrcWindow->top;
        srcEffBottom = (gctFLOAT) SrcWindow->bottom;

        /*******************************************************************
        ** Determine texture window.
        */

        if (Layer->flip)
        {
            srcEffTop    = Layer->sizeY - srcEffTop;
            srcEffBottom = Layer->sizeY - srcEffBottom;
        }

        texLeft   = srcEffLeft   / Layer->sizeX;
        texTop    = srcEffTop    / Layer->sizeY;
        texRight  = srcEffRight  / Layer->sizeX;
        texBottom = srcEffBottom / Layer->sizeY;

        /* Get source */
        a0x = texLeft;  a0y = texTop;
        a1x = texRight; a1y = texTop;
        a2x = texLeft;  a2y = texBottom;

        /* Initial trg */
        trgX0 = (gctFLOAT) Layer->input.v0.x;
        trgY0 = (gctFLOAT) Layer->input.v0.y;

        trgX1 = (gctFLOAT) Layer->input.v1.x;
        trgY1 = (gctFLOAT) Layer->input.v1.y;

        trgX2 = (gctFLOAT) Layer->input.v2.x;
        trgY2 = (gctFLOAT) Layer->input.v2.y;

        /* Calculate area */
        dx0   = trgX0 - trgX1;
        dy0   = trgY0 - trgY1;
        dx1   = trgX0 - trgX2;
        dy1   = trgY0 - trgY2;

        area  = gcoMATH_SquareRoot(
                    (dx0 * dx0 + dy0 * dy0) * (dx1 * dx1 + dy1 * dy1)
                );

        /*
            Compute dA/dx:
                dA/dx = ((A1 - A0) * (y2 - y0) + (A2 - A0) * (y0 - y1)) / area.
        */
        attrXDX.f
            = (gctFLOAT) (
                (a1x - a0x) * (trgY2 - trgY0) +
                (a2x - a0x) * (trgY0 - trgY1)
            )
            / area;

        attrYDX.f
            = (gctFLOAT) (
                (a1y - a0y) * (trgY2 - trgY0) +
                (a2y - a0y) * (trgY0 - trgY1)
            )
            / area;

        /*
            Compute dA/dy:
                dA/dy = (A2 - A0) * (x1 - x0) + (A1 - A0) * (x0 - x2)) / area.
        */
        attrXDY.f
            = (gctFLOAT) (
                (a2x - a0x) * (trgX1 - trgX0) +
                (a1x - a0x) * (trgX0 - trgX2)
            )
            / area;

        attrYDY.f
            = (gctFLOAT) (
                (a2y - a0y) * (trgX1 - trgX0) +
                (a1y - a0y) * (trgX0 - trgX2)
            )
            / area;

        /*
            Compute initial value at A0 (top left):
                A = maxY_a + dx * (dA / dx) + dy * (dA / dy)
        */
        attrX.f   = a0x + (TrgEffective->left - trgX0) * attrXDX.f
                  + (TrgEffective->top  - trgY0) * attrXDY.f;

        attrY.f   = a0y + (TrgEffective->top  - trgY0) * attrYDY.f
                  + (TrgEffective->left - trgX0) * attrYDX.f;

        /* Set the attributes. */
        gcmiCOMPOSITIONSTATE(
            buffer, 0x0C18 + sampler, attrX.u
            );

        gcmiCOMPOSITIONSTATE(
            buffer, 0x0C30 + sampler, attrY.u
            );

        gcmiCOMPOSITIONSTATE(
            buffer, 0x0C20 + sampler, attrXDX.u
            );

        gcmiCOMPOSITIONSTATE(
            buffer, 0x0C38 + sampler, attrYDX.u
            );

        gcmiCOMPOSITIONSTATE(
            buffer, 0x0C28 + sampler, attrXDY.u
            );

        gcmiCOMPOSITIONSTATE(
            buffer, 0x0C40 + sampler, attrYDY.u
            );

        /***********************************************************************
        ** Set the texture sampler.
        */

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): layer=0x%08X type=%d format=%d.\n",
            __FUNCTION__, __LINE__,
            (gctUINT32) Layer, Layer->type, Layer->format
            );

        /* Determine surface alignment and addressing. */
        switch (Layer->type)
        {
        case gcvSURF_RENDER_TARGET:
            if (Layer->surface->superTiled)
            {
                if (Hardware->pixelPipes == 1)
                {
                    srcAddressing = 0x0;
                    srcAlignment = 0x2;

                    gcmTRACE_ZONE(
                        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                        "%s(%d): srcAddressing=%d srcAlignment=%d\n",
                        __FUNCTION__, __LINE__,
                        srcAddressing, srcAlignment
                        );
                }
                else
                {
                    srcAddressing = 0x0;
                    srcAlignment = 0x4;

                    gcmTRACE_ZONE(
                        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                        "%s(%d): srcAddressing=%d srcAlignment=%d\n",
                        __FUNCTION__, __LINE__,
                        srcAddressing, srcAlignment
                        );
                }
            }
            else
            {
                if (Hardware->pixelPipes == 1)
                {
                    srcAddressing = 0x0;
                    srcAlignment = 0x1;

                    gcmTRACE_ZONE(
                        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                        "%s(%d): srcAddressing=%d srcAlignment=%d\n",
                        __FUNCTION__, __LINE__,
                        srcAddressing, srcAlignment
                        );
                }
                else
                {
                    srcAddressing = 0x0;
                    srcAlignment = 0x3;

                    gcmTRACE_ZONE(
                        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                        "%s(%d): srcAddressing=%d srcAlignment=%d\n",
                        __FUNCTION__, __LINE__,
                        srcAddressing, srcAlignment
                        );
                }
            }
            break;

        case gcvSURF_TEXTURE:
            switch (Layer->surface->tiling)
            {
            case gcvLINEAR:
                srcAddressing = 0x3;
                srcAlignment  = ~0U;
                break;

            case gcvTILED:
                /* Textures can be better aligned. */
                hAlignmentAvailable = ((((gctUINT32) (Hardware->chipMinorFeatures1)) >> (0 ? 20:20) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))));

                srcAddressing = 0x0;
                srcAlignment  = hAlignmentAvailable
                                  ? 0x1
                                  : 0x0;

                break;

            case gcvSUPERTILED:
                srcAddressing = 0x0;
                srcAlignment = 0x2;
                break;

            case gcvMULTI_TILED:
                srcAddressing = 0x0;
                srcAlignment = 0x3;
                break;

            case gcvMULTI_SUPERTILED:
                srcAddressing = 0x0;
                srcAlignment = 0x4;
                break;

            default:
                gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
            }

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                "%s(%d): srcAddressing=%d srcAlignment=%d\n",
                __FUNCTION__, __LINE__,
                srcAddressing, srcAlignment
                );

            break;

        case gcvSURF_BITMAP:
            if (Layer->surface->tiling != gcvLINEAR)
            {
                gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
            }

            srcAddressing = 0x3;
            srcAlignment  = ~0U;

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                "%s(%d): srcAddressing=%d srcAlignment=%d\n",
                __FUNCTION__, __LINE__,
                srcAddressing, srcAlignment
                );
            break;

        default:
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        /***********************************************************************
        ** Program the surface configuration.
        */

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): samples=%dx%d.\n",
            __FUNCTION__, __LINE__,
            Layer->samplesX,
            Layer->samplesY
            );

        /* Determine texture sizes. */
        srcSizeX = Layer->samplesX * Layer->sizeX;
        srcSizeY = Layer->samplesY * Layer->sizeY;

        /* Determine tile status. */
        if (Layer->surface->tileStatusNode.valid && !Layer->surface->tileStatusDisabled)
        {
            srcTileStatus1 = 0x1;
            srcTileStatus2 = 0x1;
        }
        else
        {
            srcTileStatus1 = 0x0;
            srcTileStatus2 = 0x0;
        }

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): srcSizeX=%d srcSizeY=%d srcStride=%d.\n",
            __FUNCTION__, __LINE__,
            srcSizeX, srcSizeY, Layer->stride
            );

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C48 + sampler,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (Layer->format) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:5) - (0 ? 7:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:5) - (0 ? 7:5) + 1))))))) << (0 ? 7:5))) | (((gctUINT32) ((gctUINT32) (srcAlignment) & ((gctUINT32) ((((1 ? 7:5) - (0 ? 7:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:5) - (0 ? 7:5) + 1))))))) << (0 ? 7:5)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (srcAddressing) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Layer->stride) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13)))
            );

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C50 + sampler,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (srcSizeX) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (srcSizeY) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)))
            );

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C58 + sampler,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (srcSizeX) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (srcTileStatus1) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)))
            );

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C60 + sampler,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (srcSizeY) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)))
            );

        /***********************************************************************
        ** Program the surface location.
        */

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C80 + sampler * 8,
            Layer->surface->node.physical
            );

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): srcPhysical1=0x%08X.\n",
            __FUNCTION__, __LINE__,
            Layer->surface->node.physical
            );

        if (Hardware->pixelPipes > 1)
        {
            if (Layer->type == gcvSURF_RENDER_TARGET)
            {
                gctINT height, half;

                height = gcmALIGN(
                    Layer->surface->alignedHeight / 2, Layer->surface->superTiled ? 64 : 4
                    );

                half = height * Layer->stride;

                /* Extra pages needed to offset sub-buffers to different banks. */
#if gcdENABLE_BANK_ALIGNMENT
                gcmONERROR(gcoSURF_GetBankOffsetBytes(
                    gcvNULL,
                    Layer->type, half, &bankOffsetBytes
                    ));

                /* If surface doesn't have enough padding, then don't offset it. */
                if (Layer->surface->size <
                    ((Layer->surface->alignedHeight * Layer->surface->stride)
                    + bankOffsetBytes))
                {
                    bankOffsetBytes = 0;
                }
#endif

                half += bankOffsetBytes;

                gcmiCOMPOSITIONSTATE(
                    buffer,
                    0x0C80 + sampler * 8 + 1,
                    Layer->surface->node.physical + half
                    );

                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                    "%s(%d): srcPhysical2=0x%08X (half).\n",
                    __FUNCTION__, __LINE__,
                    Layer->surface->node.physical + half
                    );
            }
            else
            {
                gcmiCOMPOSITIONSTATE(
                    buffer,
                    0x0C80 + sampler * 8 + 1,
                    Layer->surface->node.physical
                    );

                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                    "%s(%d): srcPhysical2=0x%08X.\n",
                    __FUNCTION__, __LINE__,
                    Layer->surface->node.physical
                    );
            }
        }

        /***********************************************************************
        ** Set the tile status.
        */

        if (srcTileStatus2 != 0x0)
        {
            gcmONERROR(_TranslateCompositionTileStatusFormat(
                Layer->surface->format, &srcTileFormat
                ));
        }
        else
        {
            srcTileFormat = ~0U;
        }

        gcmiCOMPOSITIONSTATE(
            buffer,
            0x0C68 + sampler,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (srcTileStatus2) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (srcTileFormat) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
            );

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): srcTileStatus=%d srcTileFormat=%d.\n",
            __FUNCTION__, __LINE__,
            srcTileStatus2, srcTileFormat
            );

        if (srcTileStatus2 == 0x1)
        {
            gcmiCOMPOSITIONSTATE(
                buffer,
                0x0C70 + sampler,
                Layer->surface->tileStatusNode.physical
                );

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                "%s(%d): tileStatusNode.physical=0x%08X.\n",
                __FUNCTION__, __LINE__,
                Layer->surface->tileStatusNode.physical
                );

            gcmiCOMPOSITIONSTATE(
                buffer,
                0x0C78 + sampler,
                Layer->surface->clearValue
                );
        }
    }

OnError:
    gcmFOOTER();

    /* Return the status. */
    return status;
}

static gceSTATUS
_SetShader(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_STATE_BUFFER_PTR Buffer,
    IN gctUINT32 Data0,
    IN gctUINT32 Data1,
    IN gctUINT32 Data2,
    IN gctUINT32 Data3
    )
{
    /* Set the four instruction double words. */
    gcmiCOMPOSITIONSTATE(Buffer, Resources->currPCaddress, Data0);
    Resources->currPCaddress += 1;

    gcmiCOMPOSITIONSTATE(Buffer, Resources->currPCaddress, Data1);
    Resources->currPCaddress += 1;

    gcmiCOMPOSITIONSTATE(Buffer, Resources->currPCaddress, Data2);
    Resources->currPCaddress += 1;

    gcmiCOMPOSITIONSTATE(Buffer, Resources->currPCaddress, Data3);
    Resources->currPCaddress += 1;

    /* Advance the index to the next instruction. */
    Resources->currPC += 1;

    /* Success. */
    return gcvSTATUS_OK;
}

static gceSTATUS
_LoadLayer(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_LAYER_PTR SrcLayer
    )
{
    gceSTATUS status;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT requiredSize = 0;

    gcmHEADER_ARG(
        "Hardware=0x%08X Resources=0x%08X SrcLayer=0x%08X",
        Hardware, Resources, SrcLayer
        );

    /* Determine the required buffer size. */
    if (SrcLayer->input.operation == gcvCOMPOSE_CLEAR)
    {
        /* MOV */
        requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4;
    }
    else if (SrcLayer->input.operation == gcvCOMPOSE_LAYER)
    {
        /* TEXLD */
        requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4;

        if (SrcLayer->modulateAlpha)
        {
            /* MUL */
            requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4;
        }

        if (SrcLayer->replaceAlpha)
        {
            /* SELECT */
            requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4;
        }
    }

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    /* Are we in clear? */
    if (SrcLayer->input.operation == gcvCOMPOSE_CLEAR)
    {
        gctUINT clearColorRegister
            = Resources->constOffset
            + SrcLayer->constRegister;

        /* mov TempRegister.xyzw, ClearColor.xyzw */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MOV r%d, " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp ,gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",clearColorRegister,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),0, 0,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (clearColorRegister) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));
    }

    /* Are we in dim? */
    else if (SrcLayer->input.operation == gcvCOMPOSE_DIM)
    {
        /* Nothing to load here. */
        status = gcvSTATUS_OK;
    }

    /* No, regular layer. */
    else
    {
       /* Determine global alpha source. */
       gctUINT alphaAddress = Resources->constOffset + SrcLayer->constRegister / 4;
       gctUINT alphaSwizzle = _compSwizzle[SrcLayer->constRegister % 4];


        /* texld TempRegister.xyzw, Sampler, TempRegister.xy */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] TEXLD r%d, s%d, " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp , SrcLayer->samplerNumber,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYYY));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x18 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)))/* Sampler. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) ((gctUINT32) (SrcLayer->samplerNumber) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3)))/* Coord.xy */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYYY) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))),0));

        if (SrcLayer->modulateAlpha)
        {
            /* FIXME
             * If premultiplied (Currently):
             *     C = Cs * Amod
             *     A = As * Amod
             * Else
             *     C = Cs
             *     A = As * Amod
             */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MUL r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",alphaAddress,gcmiSOURCESWIZZLE(alphaSwizzle));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Temp. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Const. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (alphaAddress) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (alphaSwizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))));
        }

        /* If the layer has no alpha channel, get it from the constant. */
        if (SrcLayer->replaceAlpha)
        {
            /* select TempRegister.w, TempRegister.w, Aglb, TempRegister.w */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] SELECT.""NZ""r%d, "gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "d\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_WWWW),gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",alphaAddress,gcmiSOURCESWIZZLE(alphaSwizzle),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_WWWW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (gcSL_ENABLE_W ) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0B & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))),/* Check value. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_WWWW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Select1 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (alphaAddress) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (alphaSwizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Select2 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_WWWW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));
        }
    }

OnError:
    gcmFOOTER();

    /* Return the status. */
    return status;
}

static gceSTATUS
_BlendLayer(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_LAYER_PTR SrcLayer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT requiredSize = 0;

    gcmHEADER_ARG(
        "Hardware=0x%08X Resources=0x%08X SrcLayer=0x%08X",
        Hardware, Resources, SrcLayer
        );

    /* Determine the required buffer size. */
    if (SrcLayer->input.operation == gcvCOMPOSE_DIM)
    {
        if (SrcLayer->modulateAlpha)
        {
            /* MUL */
            requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4;
        }
    }
    else
    {
        if (SrcLayer->input.enableBlending)
        {
            /* MAD / ADD */
            requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4 * 2;
        }
        else
        {
            if (!SrcLayer->input.premultiplied)
            {
                /* MUL */
                requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4;
            }

            if (SrcLayer->samplerNumber > 0)
            {
                /* SELECT */
                requiredSize += (gcmSIZEOF(gctUINT32) * 2) * 4;
            }
        }
    }

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    /* Are we in dim? */
    if (SrcLayer->input.operation == gcvCOMPOSE_DIM)
    {
        /*
         * FIXME: DIM opertaion should be as follows:
         * C = 0  + Cd * (1 - Ag)  = 0  + Cd - Cd * Ag
         * A = Ag + Ad * (1 - Ag)  = Ag + Cd - Cd * Ag
         *
         * Currently:
         * C = Cg * (1 - Ag)       = Cd - Cd * Ag
         * A = Ag * (1 - Ag)       = Cd - Cd * Ag
         */
        if (SrcLayer->modulateAlpha)
        {
            /* Determine modulation alpha source. */
            gctUINT alphaAddress = Resources->constOffset + SrcLayer->constRegister / 4;
            gctUINT alphaSwizzle = _compSwizzle[SrcLayer->constRegister % 4];

            /* C = Cd * Amod
               A = Ad * Amod */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MUL r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->prevTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",alphaAddress,gcmiSOURCESWIZZLE(alphaSwizzle));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Temp. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Const. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (alphaAddress) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (alphaSwizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))));
        }
        else
        {
            Resources->currTemp = Resources->prevTemp;
        }
    }

    /* Clear and regular layer. */
    else
    {
        /*
            The blending equation is C = Cs * S + Cd * D,
            where Cs is the source layer and Cd is the layer beneath it.
        */
        if (SrcLayer->input.enableBlending)
        {
            /* Cd - Cd * As
               Ad - Ad * As */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MAD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->prevTemp ,gcmiSOURCENEG(1),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->prevTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_WWWW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->prevTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Source1. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Source2. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_WWWW) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Source3. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

            if (SrcLayer->input.premultiplied)
            {
                /* C = Cs + [Cd - Cd * As]
                   A = As + [Ad - Ad * As] */
                gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] ADD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->prevTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (gcSL_ENABLE_XYZW ) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Source1. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))),/* Source2. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));
            }
            else
            {
                /* C = Cs * As + [Cd - Cd * As]
                   A = As * As + [Ad - Ad * As] */
                gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MAD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_WWWW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->prevTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Source1. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Source2. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_WWWW) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Source3. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));
            }
        }
        else
        {
            if (!SrcLayer->input.premultiplied)
            {
                /* C = Cs * As
                   A = As * As */
                gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MUL r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_WWWW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Temp. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Const. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_WWWW) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))));
            }

            /* If this is the first layer, nothing to select from. */
            if (SrcLayer->samplerNumber > 0)
            {
                /* select C, Cs.w, Cs, Cd */
                gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] SELECT.""NZ""r%d, "gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "d\n",__FUNCTION__, __LINE__,Resources->currPC,Resources->currTemp,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_WWWW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->currTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",Resources->prevTemp,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (gcSL_ENABLE_XYZW ) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0B & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))),/* Check value. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_WWWW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Select1 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Resources->currTemp) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Select2 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (Resources->prevTemp) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));
            }
        }
    }

OnError:
    gcmFOOTER();

    /* Return the status. */
    return status;
}

static gceSTATUS
_DoCompose(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_SET_PTR Set,
    IN gcsCOMPOSITION_LAYER_PTR Target
    )
{
    gceSTATUS status;
    gctINT boundingRectWidth;
    gctINT boundingRectHeight;
    gcsCOMPOSITION_NODE_PTR currNode;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT requiredSize;

    gcmHEADER_ARG(
        "Hardware=0x%08X Resources=0x%08X "
        "Target=0x%08X Set=0x%08X",
        Hardware, Resources, Target, Set
        );

    /***************************************************************************
    ** Initialize the state.
    */

    /* Initialize resources. */
    gcmONERROR(_InitResources(Hardware, Resources));

    /* Initialize internal state. */
    gcmONERROR(_InitCompositionState(Hardware, Resources));

    /* Set the target. */
    gcmONERROR(_SetCompositionTarget(Hardware, Resources, Target->surface));

    /***************************************************************************
    ** Set destination coordinates.
    */

    /* Determine the required buffer size. */
    requiredSize = (gcmSIZEOF(gctUINT32) * 2) * 2;

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    boundingRectWidth  = Set->trgRect.right  - Set->trgRect.left;
    boundingRectHeight = Set->trgRect.bottom - Set->trgRect.top;

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C00,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (Set->trgRect.left) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (Set->trgRect.top) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)))
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C01,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0))) | (((gctUINT32) ((gctUINT32) (boundingRectWidth) & ((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16))) | (((gctUINT32) ((gctUINT32) (boundingRectHeight) & ((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16)))
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): Bounding rectangle %d,%d; %dx%d\n",
        __FUNCTION__, __LINE__,
        Set->trgRect.left, Set->trgRect.top,
        boundingRectWidth, boundingRectHeight
        );

    /***************************************************************************
    ** Compose layers.
    */

    /* Generate states for all layers. */
    currNode = Set->nodeHead;

    while (currNode != gcvNULL)
    {
        /* Did we reach the number of supported layers? */
        if (Resources->currSampler == 8)
        {
            /* Add the composition trigger. */
            gcmONERROR(_TriggerComposition(Hardware, Resources));

            /* Initialize resources. */
            gcmONERROR(_InitResources(Hardware, Resources));
        }

        /* Sample the target if the first layer requires it. */
        if ((Resources->currSampler == 0) && currNode->layer->needPrevious)
        {
            /* Set the sampler configuration. */
            gcmONERROR(_SetCompositionSampler(
                Hardware, Resources, Target,
                &Set->trgRect, /* Source window. */
                &Set->trgRect, /* Target window. */
                &Set->trgRect                   /* Bounding rect. */
                ));

            /* Load the target pixel. */
            gcmONERROR(_LoadLayer(Hardware, Resources, Target));

            /* Advance the current temporary register. */
            Resources->prevTemp  = Resources->currTemp;
            Resources->currTemp += 1;
        }

        /* Set the sampler configuration. */
        gcmONERROR(_SetCompositionSampler(
            Hardware, Resources, currNode->layer,
            &currNode->layer->input.srcRect, /* Source window. */
            &currNode->layer->input.trgRect, /* Target window. */
            &Set->trgRect                       /* Bounding rect. */
            ));

        /* Load the layer pixel. */
        gcmONERROR(_LoadLayer(Hardware, Resources, currNode->layer));

        /* Blend with the previous layer. */
        gcmONERROR(_BlendLayer(Hardware, Resources, currNode->layer));

        /* Advance the current temporary register. */
        Resources->prevTemp  = Resources->currTemp;
        Resources->currTemp += 1;

        /* Advance to the next node. */
        currNode = currNode->next;
    }

    /* Program shader control. */
    gcmONERROR(_TriggerComposition(Hardware, Resources));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_GenerateFilter(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_LAYER_PTR SrcLayer,
    IN gcsiFILTER_PARAMETERS_PTR Parameters,
    IN gctBOOL Horizontal
    )
{
    gceSTATUS status;

    gctUINT center, current, temp;
    gctUINT color, sum;

    gctUINT coordSwizzle;
    gctUINT coordEnable;
    gcsiCONST_ALLOCATOR_PTR coordStep;

    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT requiredSize;

    gcmHEADER_ARG(
        "Hardware=0x%08X Resources=0x%08X SrcLayer=0x%08X Parameters=0x%08X",
        Hardware, Resources, SrcLayer, Parameters
        );

    /* Determine the required buffer size. */
    requiredSize = (gcmSIZEOF(gctUINT32) * 2) * 4 * 15;

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    /***********************************************************************
    ** Allocate registers.
    */

    /* Coordinates of the center pixel. */
    center = Resources->currTemp;
    Resources->currTemp += 1;

    /* Coordinates of the current pixel. */
    current = Resources->currTemp;
    Resources->currTemp += 1;

    /* Temporary value register. */
    temp = Resources->currTemp;
    Resources->currTemp += 1;

    /* Currently sampled color. */
    color = Resources->currTemp;
    Resources->currTemp += 1;

    /* The result sum. */
    sum = Resources->currTemp;
    Resources->currTemp += 1;

    /***********************************************************************
    ** Determine directional parameters.
    */

    if (Horizontal)
    {
        coordSwizzle =  gcSL_SWIZZLE_XXXX;
        coordEnable  =  gcSL_ENABLE_X;
        coordStep    = &Parameters->stepX;
    }
    else
    {
        coordSwizzle =  gcSL_SWIZZLE_YYYY;
        coordEnable  =  gcSL_ENABLE_Y;
        coordStep    = &Parameters->stepY;
    }

    /***********************************************************************
    ** Reset the sum.
    */

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MOV r%d, " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,sum ,gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",Parameters->zero.address,gcmiSOURCESWIZZLE(Parameters->zero.swizzle));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (sum ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),0, 0,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (Parameters->zero.address) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (Parameters->zero.swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

    /***********************************************************************
    ** Set the initial coordinate.
    */

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MOV r%d, " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,current ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",center,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYYY));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (current ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),0, 0,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (center) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYYY) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

    /***********************************************************************
    ** Compute the sums of the left part of the kernel including the
    ** center.
    */

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] REP\n",Resources->currPC,__FUNCTION__, __LINE__);gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x1F & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))),0,/* Loop info. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Parameters->loop1Info.address) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (Parameters->loop1Info.swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Label. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7))) | (((gctUINT32) ((gctUINT32) (Resources->currPC + 6 ) & ((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7)))));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] TEXLD r%d, s%d, " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,color , SrcLayer->samplerNumber,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",current,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYYY));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x18 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (color ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)))/* Sampler. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) ((gctUINT32) (SrcLayer->samplerNumber) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3)))/* Coord.xy */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYYY) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))),0));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MAD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,sum ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",color,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",Parameters->coefficient0.address,gcmiSOURCESWIZZLE(Parameters->coefficient0.swizzle),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",sum,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (sum ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Source1. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (color) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Source2. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Parameters->coefficient0.address) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (Parameters->coefficient0.swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x5) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Source3. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (sum) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] ADD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,temp,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",current,gcmiSOURCESWIZZLE(coordSwizzle),gcmiSOURCENEG(1),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",coordStep->address,gcmiSOURCESWIZZLE(coordStep->swizzle));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (temp) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (coordEnable ) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Source1. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (coordSwizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))),/* Source2. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (coordStep->address) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (coordStep->swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] SELECT.""LE""r%d, "gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "d\n",__FUNCTION__, __LINE__,Resources->currPC,current,gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",Parameters->zero.address,gcmiSOURCESWIZZLE(Parameters->zero.swizzle),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",temp,gcmiSOURCESWIZZLE(coordSwizzle),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",current,gcmiSOURCESWIZZLE(coordSwizzle));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (coordEnable ) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x04 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))),/* Check value. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Parameters->zero.address) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (Parameters->zero.swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Select1 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (temp) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (coordSwizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Select2 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (coordSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] ENDREP\n",__FUNCTION__, __LINE__,Resources->currPC);gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x20 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))),0,/* Loop info. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Parameters->loop1Info.address) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (Parameters->loop1Info.swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))), ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* RepeatLabel. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7))) | (((gctUINT32) ((gctUINT32) (Resources->currPC - 4 ) & ((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7)))));

    /***********************************************************************
    ** Set the initial coordinate.
    */

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MOV r%d, " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,current ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",center,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYYY));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (current ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),0, 0,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (center) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYYY) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

    /***********************************************************************
    ** Compute the sums of the right part of the kernel.
    */

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] REP\n",Resources->currPC,__FUNCTION__, __LINE__);gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x1F & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))),0,/* Loop info. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Parameters->loop2Info.address) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (Parameters->loop2Info.swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Label. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7))) | (((gctUINT32) ((gctUINT32) (Resources->currPC + 6 ) & ((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7)))));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] ADD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,temp,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",current,gcmiSOURCESWIZZLE(coordSwizzle),gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",coordStep->address,gcmiSOURCESWIZZLE(coordStep->swizzle));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (temp) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (coordEnable ) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Source1. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (coordSwizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))),/* Source2. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (coordStep->address) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (coordStep->swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] SELECT.""GE""r%d, "gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "d\n",__FUNCTION__, __LINE__,Resources->currPC,current,gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",Parameters->one.address,gcmiSOURCESWIZZLE(Parameters->one.swizzle),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",temp,gcmiSOURCESWIZZLE(coordSwizzle),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",current,gcmiSOURCESWIZZLE(coordSwizzle));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (coordEnable ) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))),/* Check value. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (Parameters->one.address) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (Parameters->one.swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Select1 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (temp) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (coordSwizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Select2 */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (coordSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] TEXLD r%d, s%d, " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,color , SrcLayer->samplerNumber,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",current,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYYY));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x18 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (color ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)))/* Sampler. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) ((gctUINT32) (SrcLayer->samplerNumber) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3)))/* Coord.xy */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (current) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYYY) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))),0));

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] MAD r%d, " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT ", " gcmiSOURCEFORMAT "\n",__FUNCTION__, __LINE__,Resources->currPC,sum ,gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",color,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW),gcmiSOURCENEG(0),(0x2 == 0x0) ? "r" : (0x2 == 0x2) ? "c" : "<?>",Parameters->coefficient0.address,gcmiSOURCESWIZZLE(Parameters->coefficient0.swizzle),gcmiSOURCENEG(0),(0x0 == 0x0) ? "r" : (0x0 == 0x2) ? "c" : "<?>",sum,gcmiSOURCESWIZZLE(gcSL_SWIZZLE_XYZW));gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))/* Destination. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (sum ) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))),/* Source1. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (color) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))/* Source2. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Parameters->coefficient0.address) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (Parameters->coefficient0.swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x5) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))),((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* Source3. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (sum) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,"%s(%d): [%d] ENDREP\n",__FUNCTION__, __LINE__,Resources->currPC);gcmONERROR(_SetShader(Hardware, Resources, buffer,((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x20 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))),0,/* Loop info. */((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (Parameters->loop2Info.address) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (Parameters->loop2Info.swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))), ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))/* RepeatLabel. */| ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7))) | (((gctUINT32) ((gctUINT32) (Resources->currPC - 4 ) & ((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7)))));

    /* Set the output register. */
    Resources->prevTemp = sum;

OnError:
    gcmFOOTER();

    /* Return the status. */
    return status;
}

static gceSTATUS
_DoBlur(
    IN gcoHARDWARE Hardware,
    IN gcsiCOMPOSITION_RESOURCES_PTR Resources,
    IN gcsCOMPOSITION_SET_PTR Set,
    IN gcsCOMPOSITION_LAYER_PTR Layer
    )
{
    gceSTATUS status;
    gctINT boundingRectWidth;
    gctINT boundingRectHeight;
    gctINT tempWidth;
    gctINT tempHeight;
    gctINT topExtra, bottomExtra;
    gcsRECT srcWindow;

    gcsiFILTER_PARAMETERS parameters;
    gctFLOAT stepX;
    gctFLOAT stepY;
    gctUINT loop1Info;
    gctUINT loop2Info;

    gcsCOMPOSITION_LAYER_PTR tempLayer;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gctUINT requiredSize;

    gcmHEADER_ARG(
        "Hardware=0x%08X Resources=0x%08X "
        "Layer=0x%08X Set=0x%08X",
        Hardware, Resources, Layer, Set
        );

    /* Layer shortcut. */
    tempLayer = &Hardware->composition.tempLayer;

    /***************************************************************************
    ** Allocate temorary buffer.
    */

    /* Compute the size of the bounding rectangle. */
    boundingRectWidth  = Set->trgRect.right  - Set->trgRect.left;
    boundingRectHeight = Set->trgRect.bottom - Set->trgRect.top;

    /* We need to render vertically more if available to cover the extra pixels
       for the kernel. */
    topExtra    = Set->trgRect.top;
    bottomExtra = Layer->sizeY - Set->trgRect.bottom;

    if (topExtra > gcdiBLUR_KERNEL_HALF)
    {
        topExtra = gcdiBLUR_KERNEL_HALF;
    }

    if (bottomExtra > gcdiBLUR_KERNEL_HALF)
    {
        bottomExtra = gcdiBLUR_KERNEL_HALF;
    }

    /* Determine the size of the temporary surface. */
    tempWidth  = boundingRectWidth;
    tempHeight = topExtra + boundingRectHeight + bottomExtra;

    /* Allocate the surface. */
    gcmONERROR(_AllocateTemporarySurface(
        Hardware, tempWidth, tempHeight,
        Layer->surface->format, Layer->surface->type
        ));

    /***************************************************************************
    ** Initialize the state.
    */

    /* Initialize resources. */
    gcmONERROR(_InitResources(Hardware, Resources));

    /* Initialize internal state. */
    gcmONERROR(_InitCompositionState(Hardware, Resources));

    /***************************************************************************
    ** Allocate and setup constants.
    */

    gcmONERROR(_ResetConstantAllocator(Resources));

    gcmONERROR(_AllocateConstant(Resources, &parameters.loop1Info, 1));
    gcmONERROR(_AllocateConstant(Resources, &parameters.loop2Info, 1));

    gcmONERROR(_AllocateConstant(Resources, &parameters.stepX, 1));
    gcmONERROR(_AllocateConstant(Resources, &parameters.stepY, 1));

    gcmONERROR(_AllocateConstant(Resources, &parameters.zero, 1));
    gcmONERROR(_AllocateConstant(Resources, &parameters.one, 1));

    gcmONERROR(_AllocateConstant(Resources, &parameters.coefficient0, 4));
    gcmONERROR(_AllocateConstant(Resources, &parameters.coefficient1, 4));
    gcmONERROR(_AllocateConstant(Resources, &parameters.coefficient2, 4));
    gcmONERROR(_AllocateConstant(Resources, &parameters.coefficient3, 4));
    gcmONERROR(_AllocateConstant(Resources, &parameters.coefficient4, 4));

    loop1Info
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (5) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21)));

    loop2Info
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (4) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21)));

    stepX = 1.0f / boundingRectWidth;
    stepY = 1.0f / boundingRectHeight;

    /* Determine the required buffer size. */
    requiredSize = (gcmSIZEOF(gctUINT32) * 2) * 11;

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    gcmiCOMPOSITIONCONST     (Resources, buffer, parameters.loop1Info.index, loop1Info);
    gcmiCOMPOSITIONCONST     (Resources, buffer, parameters.loop2Info.index, loop2Info);

    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.stepX.index, stepX);
    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.stepY.index, stepY);

    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.zero.index, 0.0f);
    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.one.index, 1.0f);

    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.coefficient0.index, 0.16f);
    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.coefficient1.index, 0.15f);
    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.coefficient2.index, 0.12f);
    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.coefficient3.index, 0.09f);
    gcmiCOMPOSITIONFLOATCONST(Resources, buffer, parameters.coefficient4.index, 0.05f);

    /***************************************************************************
    ** Horizontal pass.
    */

    /* Set the target. */
    gcmONERROR(_SetCompositionTarget(
        Hardware, Resources, &Hardware->composition.tempBuffer
        ));

    /* Determine the required buffer size. */
    requiredSize = (gcmSIZEOF(gctUINT32) * 2) * 2;

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    /* Set the target rectangle. */
    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C00,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)))
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C01,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0))) | (((gctUINT32) ((gctUINT32) (tempWidth) & ((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16))) | (((gctUINT32) ((gctUINT32) (tempHeight) & ((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16)))
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): Bounding rectangle %d,%d; %dx%d\n",
        __FUNCTION__, __LINE__,
        0, 0, tempWidth, tempHeight
        );

    /* Set the source window. */
    srcWindow.left   = Set->trgRect.left;
    srcWindow.top    = Set->trgRect.top    - topExtra;
    srcWindow.right  = Set->trgRect.right;
    srcWindow.bottom = Set->trgRect.bottom + bottomExtra;

    /* Set target rectangle to match target window. */
    Layer->input.v0.x = tempLayer->input.trgRect.left;
    Layer->input.v0.y = tempLayer->input.trgRect.top;
    Layer->input.v1.x = tempLayer->input.trgRect.right;
    Layer->input.v1.y = tempLayer->input.trgRect.top;
    Layer->input.v2.x = tempLayer->input.trgRect.left;
    Layer->input.v2.y = tempLayer->input.trgRect.bottom;

    /* Set the sampler configuration. */
    gcmONERROR(_SetCompositionSampler(
        Hardware, Resources, Layer,
        &srcWindow,
        &tempLayer->input.trgRect,
        &tempLayer->input.trgRect
        ));

    /* Restore target rectangle. */
    Layer->input.v0.x = 0;
    Layer->input.v0.y = 0;
    Layer->input.v1.x = Layer->sizeX;
    Layer->input.v1.y = 0;
    Layer->input.v2.x = 0;
    Layer->input.v2.y = Layer->sizeY;

    /* Generate the shader for the horizontal filter. */
    gcmONERROR(_GenerateFilter(Hardware, Resources, Layer, &parameters, gcvTRUE));

    /* Program shader control. */
    gcmONERROR(_TriggerComposition(Hardware, Resources));

    /***************************************************************************
    ** Vertical pass.
    */

    /* Initialize resources. */
    gcmONERROR(_InitResources(Hardware, Resources));

    /* Set the target. */
    gcmONERROR(_SetCompositionTarget(Hardware, Resources, Layer->surface));

    /* Determine the required buffer size. */
    requiredSize = (gcmSIZEOF(gctUINT32) * 2) * 2;

    /* Make sure there is enough space in the buffer. */
    gcmONERROR(_ProbeBuffer(Hardware, requiredSize, &buffer));

    /* Set the target rectangle. */
    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C00,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (Set->trgRect.left) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (Set->trgRect.top) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)))
        );

    gcmiCOMPOSITIONSTATE(
        buffer,
        0x0C01,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0))) | (((gctUINT32) ((gctUINT32) (boundingRectWidth) & ((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16))) | (((gctUINT32) ((gctUINT32) (boundingRectHeight) & ((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16)))
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
        "%s(%d): Bounding rectangle %d,%d; %dx%d\n",
        __FUNCTION__, __LINE__,
        Set->trgRect.left, Set->trgRect.top,
        boundingRectWidth, boundingRectHeight
        );

    /* Set the source window. */
    srcWindow.left        = tempLayer->input.srcRect.left;
    srcWindow.top         = tempLayer->input.srcRect.top    + topExtra;
    srcWindow.right       = tempLayer->input.srcRect.right;
    srcWindow.bottom      = tempLayer->input.srcRect.bottom - bottomExtra;

    /* Set target rectangle. */
    tempLayer->input.v0.x = Set->trgRect.left;
    tempLayer->input.v0.y = Set->trgRect.top;
    tempLayer->input.v1.x = Set->trgRect.right;
    tempLayer->input.v1.y = Set->trgRect.top;
    tempLayer->input.v2.x = Set->trgRect.left;
    tempLayer->input.v2.y = Set->trgRect.bottom;

    /* Set the sampler configuration. */
    gcmONERROR(_SetCompositionSampler(
        Hardware, Resources,
        &Hardware->composition.tempLayer,
        &srcWindow,
        &Set->trgRect,
        &Set->trgRect
        ));

    /* Apply vertical filter. */
    gcmONERROR(_GenerateFilter(Hardware, Resources, Layer, &parameters, gcvFALSE));

    /* Program shader control. */
    gcmONERROR(_TriggerComposition(Hardware, Resources));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/******************************************************************************\
******************************* Composition API ********************************
\******************************************************************************/

gceSTATUS
gcoHARDWARE_InitializeComposition(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gctBOOL bugFixes1;
    gcsCOMPOSITION_STATE_BUFFER_PTR buffer;
    gcsCOMPOSITION_STATE_BUFFER_PTR tail = gcvNULL;
    gctINT i;

    gcmHEADER_ARG("Hardware=0x%08X", Hardware);

    /* Determine whether bug fixes #1 are present. */
    bugFixes1 = ((((gctUINT32) (Hardware->chipMinorFeatures1)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x0 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))));

    /* Determine the hardware event size. */
    Hardware->composition.eventSize = bugFixes1
        ? gcmALIGN(8 + (1 + 5) * 4, 8) /* EVENT + 5 STATES */
        : 8;

    /* We may need one alpha value per layer. */
    Hardware->composition.maxConstCount = 8;

    /* Determine maximum shader length. */
    Hardware->composition.maxShaderLength

        /* One texture load per layer. */
        = (1 * 8)

        /* May need to replace alpha value with a MOV. */
        + (1 * 8)

        /* Layer blending; usually [MAD + ADD] */
        + (2 * 8);

    /* Determine the constant base. */
    Hardware->composition.constBase
        = Hardware->psConstMax - Hardware->composition.maxConstCount;

    /* Determine the instruction base. */
    Hardware->composition.instBase
        = Hardware->psInstMax - Hardware->composition.maxShaderLength;

    /* Adjust the total number of constants available to regular programs. */
    if (Hardware->unifiedConst)
    {
        Hardware->vsConstMax -= Hardware->composition.maxConstCount;
        Hardware->psConstMax -= Hardware->composition.maxConstCount;
        Hardware->constMax   -= Hardware->composition.maxConstCount;
    }
    else
    {
        Hardware->psConstMax -= Hardware->composition.maxConstCount;
        Hardware->constMax   -= Hardware->composition.maxConstCount;
    }

    /* Adjust the total number of instructions available to regular programs. */
    if (Hardware->unifiedShader)
    {
        Hardware->vsInstMax -= Hardware->composition.maxShaderLength;
        Hardware->psInstMax -= Hardware->composition.maxShaderLength;
        Hardware->instMax   -= Hardware->composition.maxShaderLength;
    }
    else
    {
        Hardware->psInstMax -= Hardware->composition.maxShaderLength;
        Hardware->instMax   -= Hardware->composition.maxShaderLength;
    }

    /* Initialize the ring buffer. */
    for (i = 0; i < gcdCOMP_BUFFER_COUNT; i += 1)
    {
        /* Shortcut to the current buffer. */
        buffer = &Hardware->composition.compStateBuffer[i];

        /* Reset the buffer signal. */
        buffer->signal    = gcvNULL;

        /* Reset the buffer address. */
        buffer->size      = 0;
        buffer->physical  = gcvNULL;
        buffer->address   = ~0U;
        buffer->logical   = gcvNULL;

        /* Determine the size of reserved area. */
        buffer->reserve
            = gcmSIZEOF(gctUINT32) * 2 + Hardware->composition.eventSize;

        /* Reset allocation. */
        buffer->head      = gcvNULL;
        buffer->tail      = gcvNULL;
        buffer->available = 0;
        buffer->count     = 0;
        buffer->rectangle = gcvNULL;

        /* Add to the ring buffer. */
        if (tail == gcvNULL)
        {
            Hardware->composition.compStateBufferCurrent
                = &Hardware->composition.compStateBuffer[i];
        }
        else
        {
            tail->next = &Hardware->composition.compStateBuffer[i];
        }

        tail = &Hardware->composition.compStateBuffer[i];
        tail->next = Hardware->composition.compStateBufferCurrent;
    }

    /* Initialize container storage. */
    gcmONERROR(gcsCONTAINER_Construct(
        &Hardware->composition.freeSets,
        gcdiTARGET_ITEM_COUNT,
        gcmSIZEOF(gcsCOMPOSITION_SET)
        ));

    gcmONERROR(gcsCONTAINER_Construct(
        &Hardware->composition.freeNodes,
        gcdiNODE_ITEM_COUNT,
        gcmSIZEOF(gcsCOMPOSITION_NODE)
        ));

    gcmONERROR(gcsCONTAINER_Construct(
        &Hardware->composition.freeLayers,
        gcdiLAYER_ITEM_COUNT,
        gcmSIZEOF(gcsCOMPOSITION_LAYER)
        ));

    /* Initialize the set linked list. */
    Hardware->composition.head.next = &Hardware->composition.head;
    Hardware->composition.head.prev = &Hardware->composition.head;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_DestroyComposition(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
    gctINT i;

    gcmHEADER_ARG("Hardware=0x%08X", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Delete existing buffer. */
    gcmONERROR(_FreeTemporarySurface(Hardware, gcvTRUE));

    /* Free containers. */
    gcmONERROR(gcsCONTAINER_Destroy(&Hardware->composition.freeSets));
    gcmONERROR(gcsCONTAINER_Destroy(&Hardware->composition.freeNodes));
    gcmONERROR(gcsCONTAINER_Destroy(&Hardware->composition.freeLayers));

    /* Reset the structure. */
    Hardware->composition.enabled   = gcvFALSE;
    Hardware->composition.head.next = &Hardware->composition.head;
    Hardware->composition.head.prev = &Hardware->composition.head;

    /* Free the state buffer. */
    for (i = 0; i < gcdCOMP_BUFFER_COUNT; i += 1)
    {
        /* Destroy the buffer signal. */
        if (Hardware->composition.compStateBuffer[i].signal != gcvNULL)
        {
            gcmONERROR(gcoOS_DestroySignal(
                gcvNULL, Hardware->composition.compStateBuffer[i].signal
                ));

            /* Reset the signal. */
            Hardware->composition.compStateBuffer[i].signal = gcvNULL;
        }

        if (Hardware->composition.compStateBuffer[i].size != 0)
        {
            /* Use event to free the buffer. */
            iface.command = gcvHAL_FREE_CONTIGUOUS_MEMORY;
            iface.u.FreeContiguousMemory.bytes    = Hardware->composition.compStateBuffer[i].size;
            iface.u.FreeContiguousMemory.physical = Hardware->composition.compStateBuffer[i].physical;
            iface.u.FreeContiguousMemory.logical  = Hardware->composition.compStateBuffer[i].logical;

            gcmONERROR(gcoHARDWARE_CallEvent(&iface));

            /* Reset the buffer pointers. */
            Hardware->composition.compStateBuffer[i].size     = 0;
            Hardware->composition.compStateBuffer[i].physical = gcvNULL;
            Hardware->composition.compStateBuffer[i].address  = ~0U;
            Hardware->composition.compStateBuffer[i].logical  = gcvNULL;
        }
    }

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_CompositionBegin(
    void
    )
{
    gceSTATUS status;
    gcoHARDWARE Hardware;

    gcmHEADER();

#if _ENABLE_DUMPING
    gcoOS_SetDebugLevelZone(gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION);
#endif

    /* Retrieve the hardware object. */
    gcmGETHARDWARE(Hardware);

    /* Are we already in composition mode? */
    if (Hardware->composition.enabled)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Make sure composition is supported. */
    if (!Hardware->hwComposition)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Reset signals. */
    Hardware->composition.process = gcvNULL;
    Hardware->composition.signal1 = gcvNULL;
    Hardware->composition.signal2 = gcvNULL;

    /* Enable composition mode. */
    Hardware->composition.enabled = gcvTRUE;

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_ProbeComposition(
    gctBOOL ResetIfEmpty
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware = gcvNULL;

    gcmHEADER_ARG("ResetIfEmpty=%d", ResetIfEmpty);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Are we already in composition mode? */
    if (!hardware->composition.enabled)
    {
        status = gcvSTATUS_INVALID_REQUEST;
        goto OnError;
    }

    /* Is there anything to compose. */
    if (hardware->composition.head.next == &hardware->composition.head)
    {
        /* Reset composition enabled if requested. */
        if (ResetIfEmpty)
        {
            hardware->composition.enabled = gcvFALSE;
        }

        status = gcvSTATUS_NO_MORE_DATA;
        goto OnError;
    }

    /* We do have layers. */
    status = gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_ComposeLayer(
    IN gcsCOMPOSITION_PTR Layer
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware = gcvNULL;
    gcsCOMPOSITION_LAYER_PTR layer = gcvNULL;

    gcmHEADER_ARG("Layer=0x%08X", Layer);

    /* Retrieve the hardware object. */
    gcmGETHARDWARE(hardware);

    /* Are we already in composition mode? */
    if (!hardware->composition.enabled)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Verify the size of the structure. */
    if (Layer->structSize != gcmSIZEOF(gcsCOMPOSITION))
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Allocate new layer. */
    gcmONERROR(gcsCONTAINER_AllocateRecord(
        &hardware->composition.freeLayers,
        (gctPOINTER *) &layer
        ));

    layer->flip          = gcvFALSE;
    layer->input.trgRect = Layer->trgRect;

    /* Initialize the structure. */
    switch (Layer->operation)
    {
    case gcvCOMPOSE_CLEAR:
        layer->input.operation = gcvCOMPOSE_CLEAR;

        layer->type     = gcvSURF_TYPE_UNKNOWN;
        layer->stride   = 0;

        layer->sizeX    = 0;
        layer->sizeY    = 0;

        layer->samplesX = 1;
        layer->samplesY = 1;

        layer->format   = 0x07;
        layer->hasAlpha = gcvTRUE;

        layer->input.r = Layer->r;
        layer->input.g = Layer->g;
        layer->input.b = Layer->b;
        layer->input.a = Layer->a;

        layer->input.srcRect.left   = 0;
        layer->input.srcRect.top    = 0;
        layer->input.srcRect.right  = 0;
        layer->input.srcRect.bottom = 0;

        layer->input.alphaValue     = Layer->alphaValue;
        layer->input.premultiplied  = Layer->premultiplied;
        layer->input.enableBlending = Layer->enableBlending;

        layer->needPrevious  = layer->input.enableBlending;
        layer->replaceAlpha  = gcvFALSE;
        layer->modulateAlpha = gcvFALSE;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): layer=0x%08X scheduling CLEAR layer.\n",
            __FUNCTION__, __LINE__, layer
            );

        break;

    case gcvCOMPOSE_BLUR:
        layer->input.operation = gcvCOMPOSE_BLUR;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): layer=0x%08X scheduling BLUR layer.\n",
            __FUNCTION__, __LINE__, layer
            );

        break;

    case gcvCOMPOSE_DIM:
        layer->input.operation = gcvCOMPOSE_DIM;

        layer->type     = gcvSURF_TYPE_UNKNOWN;
        layer->stride   = 0;

        layer->sizeX    = 0;
        layer->sizeY    = 0;

        layer->samplesX = 1;
        layer->samplesY = 1;

        layer->format   = 0x07;
        layer->hasAlpha = gcvTRUE;

        layer->input.srcRect = Layer->trgRect;

        layer->input.alphaValue     = 255 - Layer->alphaValue;
        layer->input.premultiplied  = gcvTRUE;
        layer->input.enableBlending = gcvFALSE;

        /* Not replacing alpha. */
        layer->replaceAlpha = gcvFALSE;

        /* Multiply the sampled color by the global alpha value if it has
           alpha channel and global alpha value is not 1. */
        layer->modulateAlpha = (layer->input.alphaValue != 0xFF);

        /* Dimming the previous layer. */
        layer->needPrevious = gcvTRUE;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): layer=0x%08X scheduling DIM layer.\n",
            __FUNCTION__, __LINE__, layer
            );

        break;

    case gcvCOMPOSE_LAYER:
        layer->input.operation = gcvCOMPOSE_LAYER;

        layer->surface  = &Layer->layer->info;
        layer->type     = layer->surface->type;
        layer->stride   = layer->surface->stride;

        layer->sizeX    = layer->surface->rect.right;
        layer->sizeY    = layer->surface->rect.bottom;

        layer->samplesX = layer->surface->samples.x;
        layer->samplesY = layer->surface->samples.y;

        /* Convert the format. */
        gcmONERROR(_TranslateSourceFormat(
            layer->surface->format, &layer->format, &layer->hasAlpha
            ));

        layer->input.srcRect = Layer->srcRect;

        layer->flip
            = (layer->surface->orientation == gcvORIENTATION_BOTTOM_TOP);

        layer->input.alphaValue     = Layer->alphaValue;
        layer->input.premultiplied  = Layer->premultiplied;
        layer->input.enableBlending = Layer->enableBlending || (layer->input.alphaValue != 0xFF);

        layer->input.v0             = Layer->v0;
        layer->input.v1             = Layer->v1;
        layer->input.v2             = Layer->v2;

        /*
         *         GL_REPLACE   GL_MODULATE
         * RGB     C = Ct       C = Ct * Cf
         *         A = Af       A = Af
         *
         * RGBA    C = Ct       C = Ct * Cf
         *         A = At       A = At * Af
         */
        /* Replace alpha channel when loading the layer if the layer has
           no alpha and global alpha is not 1. */
        /* FIXME: For RGB565 format, replace alpha channel is not needed. */
        layer->replaceAlpha = !layer->hasAlpha && (layer->input.alphaValue != 0xFF);

        /* Multiply the sampled color by the global alpha value if it has
           alpha channel and global alpha value is not 1. */
        layer->modulateAlpha = layer->input.alphaValue != 0xFF;

        /* In regular layer operation the previous layer is needed if
           blending is enabled. */
        layer->needPrevious = layer->input.enableBlending;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
            "%s(%d): layer=0x%08X scheduling PIXEL layer.\n",
            __FUNCTION__, __LINE__, layer
            );

        break;

    default:
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Process layer. */
    gcmONERROR(_ProcessLayer(hardware, layer, &layer->input.trgRect, gcvNULL));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Free the layer. */
    if (layer != gcvNULL)
    {
        gcmVERIFY_OK(gcsCONTAINER_FreeRecord(
            &hardware->composition.freeLayers, layer
            ));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_CompositionSignals(
    IN gctHANDLE Process,
    IN gctSIGNAL Signal1,
    IN gctSIGNAL Signal2
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG(
        "Process=0x%08X Signal1=0x%08X Signal2=0x%08X",
        Process, Signal1, Signal2
        );

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Store user signals. */
    hardware->composition.process = Process;
    hardware->composition.signal1 = Signal1;
    hardware->composition.signal2 = Signal2;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_CompositionEnd(
    IN gcoSURF Target,
    IN gctBOOL Synchronous
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware = gcvNULL;
    gcsCOMPOSITION_SET_PTR currSet;
    gcsCOMPOSITION_LAYER trgLayer;
    gcsiCOMPOSITION_RESOURCES resources;

    gcmHEADER_ARG("Target=0x%08X", Target);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Target, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /***************************************************************************
    ** We have to be in composition mode.
    */

    /* Are we already in composition mode? */
    if (!hardware->composition.enabled)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Is there anything to compose. */
    if (hardware->composition.head.next == &hardware->composition.head)
    {
        status = gcvSTATUS_NO_MORE_DATA;
        goto OnError;
    }

    /***************************************************************************
    ** Initialize state.
    */

    if (Synchronous)
    {
        gcmONERROR(_SetSyncronousMode(hardware));
    }
    else
    {
        gcmONERROR(_SetAsyncronousMode(hardware));
    }

    hardware->composition.initDone = gcvFALSE;
    hardware->composition.target   = gcvNULL;

    /***************************************************************************
    ** Initialize the target layer to be used as source.
    */

    /* Let's not worry about depth buffers. */
    if (Target->info.type == gcvSURF_DEPTH)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Initialize the target layer. */
    trgLayer.input.operation = gcvCOMPOSE_LAYER;

    trgLayer.surface  = &Target->info;
    trgLayer.type     =  Target->info.type;
    trgLayer.stride   =  Target->info.stride;
    trgLayer.sizeX    =  Target->info.rect.right;
    trgLayer.sizeY    =  Target->info.rect.bottom;
    trgLayer.samplesX =  Target->info.samples.x;
    trgLayer.samplesY =  Target->info.samples.y;

    trgLayer.input.v0.x           = 0;
    trgLayer.input.v0.y           = 0;
    trgLayer.input.v1.x           = trgLayer.sizeX;
    trgLayer.input.v1.y           = 0;
    trgLayer.input.v2.x           = 0;
    trgLayer.input.v2.y           = trgLayer.sizeY;

    trgLayer.input.alphaValue     = 0xFF;
    trgLayer.input.premultiplied  = gcvFALSE;
    trgLayer.input.enableBlending = gcvFALSE;

    trgLayer.flip
        = (trgLayer.surface->orientation == gcvORIENTATION_BOTTOM_TOP);

    trgLayer.replaceAlpha  = gcvFALSE;
    trgLayer.modulateAlpha = gcvFALSE;

    gcmONERROR(_TranslateSourceFormat(
        Target->info.format, &trgLayer.format, &trgLayer.hasAlpha
        ));

    /***************************************************************************
    ** Compose accumulated layers.
    */

    currSet = hardware->composition.head.next;

    while (currSet != &hardware->composition.head)
    {
        /* Blurring is done in two passes, therefore it cannot
           be combined with other layers. Compose all layers
           before blurring. */
        if (currSet->blur)
        {
            gcmONERROR(_DoBlur(
                hardware, &resources, currSet, &trgLayer
                ));
        }
        else
        {
            /* Compose the current layer. */
            gcmONERROR(_DoCompose(
                hardware, &resources, currSet, &trgLayer
                ));
        }

        currSet = currSet->next;
    }

    /* Execute accumulated composition. */
    gcmONERROR(_ExecuteBuffer(hardware));

    /* FIXME: This is to fix Holo Spiral flashing problem. */
    gcmONERROR(gcoHARDWARE_Stall());

OnError:
    if (hardware != gcvNULL)
    {
        /* Reset. */
        gcmVERIFY_OK(gcsCONTAINER_FreeAll(&hardware->composition.freeSets));
        gcmVERIFY_OK(gcsCONTAINER_FreeAll(&hardware->composition.freeNodes));
        gcmVERIFY_OK(gcsCONTAINER_FreeAll(&hardware->composition.freeLayers));

        /* Initialize the set linked list. */
        hardware->composition.head.next = &hardware->composition.head;
        hardware->composition.head.prev = &hardware->composition.head;

        /* Reset composition enable. */
        hardware->composition.enabled = gcvFALSE;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif  /* VIVANTE_NO_3D */


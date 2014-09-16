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




#ifndef __gc_hal_user_compiler_h_
#define __gc_hal_user_compiler_h_

#include "gc_hal_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************* SHADER BINARY FILE VERSION ******************/
/* the shader file version before Loadtime Constant optimization */
#define gcdSL_SHADER_BINARY_BEFORE_LTC_FILE_VERSION gcmCC(0, 0, 1, 1)

#if GC_ENABLE_LOADTIME_OPT
/* bump up version to 1.2 for Loadtime Constant optimization on 1/3/2012 */
/*#define gcdSL_SHADER_BINARY_FILE_VERSION gcmCC(0, 0, 1, 2)*/
#define gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION gcmCC(0, 0, 1, 2)
#else
/*#define gcdSL_SHADER_BINARY_FILE_VERSION gcmCC(0, 0, 1, 1)*/
#define gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION gcmCC(0, 0, 1, 1)
#endif

/* bump up version to 1.3 for struct support for variable and uniform on 3/9/2012 */
#define gcdSL_SHADER_BINARY_FILE_VERSION gcmCC(0, 0, 1, 3)

#define gcdSL_PROGRAM_BINARY_FILE_VERSION gcmCC('\0','\0','\11','\0')

/******************************************************************************\
|******************************* SHADER LANGUAGE ******************************|
\******************************************************************************/

/* Special register indices. */
#define gcSL_POSITION			((gctSIZE_T) -1)
#define gcSL_POINT_SIZE			((gctSIZE_T) -2)
#define gcSL_COLOR				((gctSIZE_T) -3)
#define gcSL_FRONT_FACING		((gctSIZE_T) -4)
#define gcSL_POINT_COORD		((gctSIZE_T) -5)
#define gcSL_POSITION_W			((gctSIZE_T) -6)
#define gcSL_DEPTH			((gctSIZE_T) -7)

/* Special code generation indices. */
#define gcSL_CG_TEMP1			112
#define gcSL_CG_TEMP1_X			113
#define gcSL_CG_TEMP1_XY		114
#define gcSL_CG_TEMP1_XYZ		115
#define gcSL_CG_TEMP1_XYZW		116
#define gcSL_CG_TEMP2			117
#define gcSL_CG_TEMP2_X			118
#define gcSL_CG_TEMP2_XY		119
#define gcSL_CG_TEMP2_XYZ		120
#define gcSL_CG_TEMP2_XYZW		121
#define gcSL_CG_TEMP3			122
#define gcSL_CG_TEMP3_X			123
#define gcSL_CG_TEMP3_XY		124
#define gcSL_CG_TEMP3_XYZ		125
#define gcSL_CG_TEMP3_XYZW		126
#define gcSL_CG_CONSTANT		127

/* 4-bit enable bits. */
#define gcdSL_TARGET_Enable					 0 : 4
/* Indexed addressing mode of type gcSL_INDEXED. */
#define gcdSL_TARGET_Indexed				 4 : 4
/* 4-bit condition of type gcSL_CONDITION. */
#define gcdSL_TARGET_Condition				 8 : 4
/* Target format of type gcSL_FORMAT. */
#define gcdSL_TARGET_Format					12 : 4

#define gcmSL_TARGET_GET(Value, Field) \
	gcmGETBITS(Value, gctUINT16, gcdSL_TARGET_##Field)

#define gcmSL_TARGET_SET(Value, Field, NewValue) \
	gcmSETBITS(Value, gctUINT16, gcdSL_TARGET_##Field, NewValue)

/* Register type of type gcSL_TYPE. */
#define gcdSL_SOURCE_Type					 0 : 3
/* Indexed register swizzle. */
#define gcdSL_SOURCE_Indexed				 3 : 3
/* Source format of type gcSL_FORMAT. */
#define gcdSL_SOURCE_Format					 6 : 2
/* Swizzle fields of type gcSL_SWIZZLE. */
#define gcdSL_SOURCE_Swizzle				 8 : 8
#define gcdSL_SOURCE_SwizzleX				 8 : 2
#define gcdSL_SOURCE_SwizzleY				10 : 2
#define gcdSL_SOURCE_SwizzleZ				12 : 2
#define gcdSL_SOURCE_SwizzleW				14 : 2

#define gcmSL_SOURCE_GET(Value, Field) \
	gcmGETBITS(Value, gctUINT16, gcdSL_SOURCE_##Field)

#define gcmSL_SOURCE_SET(Value, Field, NewValue) \
	gcmSETBITS(Value, gctUINT16, gcdSL_SOURCE_##Field, NewValue)

/* Index of register. */
#define gcdSL_INDEX_Index					 0 : 14
/* Constant value. */
#define gcdSL_INDEX_ConstValue				14 :  2

#define gcmSL_INDEX_GET(Value, Field) \
	gcmGETBITS(Value, gctUINT16, gcdSL_INDEX_##Field)

#define gcmSL_INDEX_SET(Value, Field, NewValue) \
	gcmSETBITS(Value, gctUINT16, gcdSL_INDEX_##Field, NewValue)

/* Structure that defines a gcSL instruction. */
typedef struct _gcSL_INSTRUCTION
{
	/* Opcode of type gcSL_OPCODE. */
	gctUINT16					opcode;

	/* Opcode condition and target write enable bits of type gcSL_TARGET. */
	gctUINT16					temp;

	/* 16-bit temporary register index. */
	gctUINT16					tempIndex;

	/* Indexed register for destination. */
	gctUINT16					tempIndexed;

	/* Type of source 0 operand of type gcSL_SOURCE. */
	gctUINT16					source0;

	/* 14-bit register index for source 0 operand of type gcSL_INDEX,
     * must accessed by gcmSL_INDEX_GET(source0Index, Index);
     * and 2-bit constant value to the base of the Index, must be
     * accessed by gcmSL_INDEX_GET(source0Index, ConstValue).
     */
	gctUINT16					source0Index;

	/* Indexed register for source 0 operand. */
	gctUINT16					source0Indexed;

	/* Type of source 1 operand of type gcSL_SOURCE. */
	gctUINT16					source1;

	/* 14-bit register index for source 1 operand of type gcSL_INDEX,
     * must accessed by gcmSL_INDEX_GET(source1Index, Index);
     * and 2-bit constant value to the base of the Index, must be
     * accessed by gcmSL_INDEX_GET(source1Index, ConstValue).
     */
	gctUINT16					source1Index;

	/* Indexed register for source 1 operand. */
	gctUINT16					source1Indexed;
}
* gcSL_INSTRUCTION;

/******************************************************************************\
|*********************************** SHADERS **********************************|
\******************************************************************************/

/* Structure the defines an attribute (input) for a shader. */
struct _gcATTRIBUTE
{
	/* The object. */
	gcsOBJECT					object;

	/* Index of the attribute. */
	gctUINT16					index;

	/* Type of the attribute. */
	gcSHADER_TYPE				type;

	/* Number of array elements for this attribute. */
	gctSIZE_T					arraySize;

	/* Flag to indicate this attribute is used as a texture coordinate. */
	gctBOOL						isTexture;

	/* Flag to indicate this attribute is enabeld or not. */
	gctBOOL						enabled;

	/* Assigned input register index. */
	gctINT						inputIndex;

	/* Length of the attribute name. */
	gctSIZE_T					nameLength;

	/* The attribute name. */
	char						name[1];
};

/* Sampel structure, but inside a binary. */
typedef struct _gcBINARY_ATTRIBUTE
{
	/* Type for this attribute of type gcATTRIBUTE_TYPE. */
	gctINT8						type;

	/* Flag to indicate this attribute is used as a texture coordinate. */
	gctINT8						isTexture;

	/* Number of array elements for this attribute. */
	gctINT16					arraySize;

	/* Length of the attribute name. */
	gctINT16					nameLength;

	/* The attribute name. */
	char						name[1];
}
* gcBINARY_ATTRIBUTE;

/* Structure that defines an uniform (constant register) for a shader. */
struct _gcUNIFORM
{
	/* The object. */
	gcsOBJECT					object;

	/* Index of the uniform. */
	gctUINT16					index;

    /* Corresponding Index of Program's GLUniform */
    gctUINT16                   glUniformIndex;

	/* Type of the uniform. */
	gcSHADER_TYPE				type;

    /* Precision of the uniform. */
	gcSHADER_PRECISION			precision;

	/* Flags. */
	gceUNIFORM_FLAGS            flags;

	/* Number of array elements for this uniform. */
	gctINT						arraySize;

	/* Format of element of the uniform shaderType. */
	gcSL_FORMAT				    format;

	/* Wheter the uniform is a pointer. */
	gctBOOL				        isPointer;

	/* Physically assigned values. */
	gctINT						physical;
	gctUINT8					swizzle;
	gctUINT32					address;

	/* Length of the uniform name. */
	gctSIZE_T					nameLength;

	/* The uniform name. */
	char						name[1];
};

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_UNIFORM
{
	/* Uniform type of type gcUNIFORM_TYPE. */
	gctINT16					type;

	/* Number of array elements for this uniform. */
	gctINT16					arraySize;

	/* Length of the uniform name. */
	gctINT16					nameLength;

#if GC_ENABLE_LOADTIME_OPT
    /* uniform flags */
	gctINT16					flags;

    /* Corresponding Index of Program's GLUniform */
    gctUINT16                   glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */

	/* The uniform name. */
	char						name[1];
}
* gcBINARY_UNIFORM;

/* Same structure, but inside a binary with more variables. */
typedef struct _gcBINARY_UNIFORM_EX
{
	/* Uniform type of type gcUNIFORM_TYPE. */
	gctINT16					type;

	/* Index of the uniform. */
	gctUINT16					index;

	/* Number of array elements for this uniform. */
	gctINT16					arraySize;

	/* Flags. */
    gctUINT16                   flags;

	/* Format of element of the uniform shaderType. */
	gctUINT16				    format;

	/* Wheter the uniform is a pointer. */
	gctINT16				    isPointer;

	/* Length of the uniform name. */
	gctINT16					nameLength;

#if GC_ENABLE_LOADTIME_OPT
    /* Corresponding Index of Program's GLUniform */
    gctUINT16                   glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */

	/* The uniform name. */
	char						name[1];
}
* gcBINARY_UNIFORM_EX;


/* Structure that defines an output for a shader. */
struct _gcOUTPUT
{
	/* The object. */
	gcsOBJECT					object;

	/* Type for this output. */
	gcSHADER_TYPE				type;

	/* Number of array elements for this output. */
	gctSIZE_T					arraySize;

	/* Temporary register index that holds the output value. */
	gctUINT16					tempIndex;

	/* Converted to physical register. */
	gctBOOL						physical;

	/* Length of the output name. */
	gctSIZE_T					nameLength;

	/* The output name. */
	char						name[1];
};

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_OUTPUT
{
	/* Type for this output. */
	gctINT8						type;

	/* Number of array elements for this output. */
	gctINT8						arraySize;

	/* Temporary register index that holds the output value. */
	gctUINT16					tempIndex;

	/* Length of the output name. */
	gctINT16					nameLength;

	/* The output name. */
	char						name[1];
}
* gcBINARY_OUTPUT;

/* Structure that defines a variable for a shader. */
struct _gcVARIABLE
{
	/* The object. */
	gcsOBJECT					object;

    /* Variable category */
    gcSHADER_VAR_CATEGORY       varCategory;

    /* Only used for structure, point to either first array element for */
    /* arrayed struct or first struct element if struct is not arrayed */
    gctINT16                    firstChild;

    /* Only used for structure, point to either next array element for */
    /* arrayed struct or next struct element if struct is not arrayed */
    gctINT16                    nextSibling;

    /* Only used for structure, point to either prev array element for */
    /* arrayed struct or prev struct element if struct is not arrayed */
    gctINT16                    prevSibling;

    /* Only used for structure, point to parent _gcVARIABLE */
    gctINT16                    parent;

    union
    {
        /* Data type for this most-inner variable. */
        gcSHADER_TYPE			type;

        /* Number of element in structure if arraySize of this */
        /* struct is 1, otherwise, set it to 0 */
        gctUINT16               numStructureElement;
    }
    u;

	/* Number of array elements for this variable, at least 1 */
	gctSIZE_T					arraySize;

    /* Start temporary register index that holds the variable value. */
    gctUINT16					tempIndex;

	/* Length of the variable name. */
	gctSIZE_T					nameLength;

	/* The variable name. */
	char						name[1];

};

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_VARIABLE
{
     union
     {
         /* Data type for this most-inner variable. */
         gctINT8			        type;
 
         /* Number of element in structure if arraySize of this */ 
         /* struct is 1, otherwise, set it to 0 */
         gctUINT8                numStructureElement;
     }
     u;
 
 	/* Number of array elements for this variable, at least 1 */
 	gctINT8					    arraySize;
 
     /* Start temporary register index that holds the variable value. */
     gctUINT16					tempIndex;
 
 	/* Length of the variable name. */
 	gctINT16					nameLength;
 
    /* Variable category */
     gctINT16                    varCategory;

    /* Only used for structure, point to either first array element for */
    /* arrayed struct or first struct element if struct is not arrayed */
    gctINT16                    firstChild;

    /* Only used for structure, point to either next array element for */
    /* arrayed struct or next struct element if struct is not arrayed */
    gctINT16                    nextSibling;

    /* Only used for structure, point to either prev array element for */
    /* arrayed struct or prev struct element if struct is not arrayed */
    gctINT16                    prevSibling;

    /* Only used for structure, point to parent _gcVARIABLE */
    gctINT16                    parent;

	/* The variable name. */
	char						name[1];
}
* gcBINARY_VARIABLE;

typedef struct _gcsFUNCTION_ARGUMENT
{
	gctUINT16					index;
	gctUINT8					enable;
	gctUINT8					qualifier;
}
gcsFUNCTION_ARGUMENT,
* gcsFUNCTION_ARGUMENT_PTR;

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_ARGUMENT
{
	gctUINT16					index;
	gctUINT8					enable;
	gctUINT8					qualifier;
}
* gcBINARY_ARGUMENT;

struct _gcsFUNCTION
{
	gcsOBJECT					object;

	gctSIZE_T					argumentArrayCount;
	gctSIZE_T					argumentCount;
	gcsFUNCTION_ARGUMENT_PTR			arguments;

	gctUINT16					label;

	/* Local variables. */
	gctSIZE_T					variableCount;
	gcVARIABLE *					variables;

	gctUINT						codeStart;
	gctUINT						codeCount;

	gctSIZE_T					nameLength;
	char						name[1];
};

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_FUNCTION
{
	gctINT16					argumentCount;
	gctINT16					variableCount;
	gctUINT16					codeStart;
	gctUINT16					codeCount;

	gctUINT16					label;

	gctINT16					nameLength;
	char						name[1];
}
* gcBINARY_FUNCTION;

typedef struct _gcsIMAGE_SAMPLER
{
	/* Kernel function argument # associated with the image passed to the kernel function */
	gctUINT8				imageNum;

	/* Sampler type either passed in as a kernel function argument which will be an argument #
            or
           defined as a constant variable inside the program which will be an unsigend integer value*/
	gctBOOL					isConstantSamplerType;

	gctUINT32				samplerType;
}
gcsIMAGE_SAMPLER,
* gcsIMAGE_SAMPLER_PTR;

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_IMAGE_SAMPLER
{
	/* index to uniform array associated with the sampler */
	gctUINT16				uniformIndex;
	gctBOOL					isConstantSamplerType;

	/* Kernel function argument # associated with the image passed to the kernel function */
	gctUINT8				imageNum;

	/* Sampler type either passed in as a kernel function argument which will be an argument #
            or
           defined as a constant variable inside the program which will be an unsigend integer value*/
	gctUINT32				samplerType;
}
* gcBINARY_IMAGE_SAMPLER;

typedef struct _gcsKERNEL_FUNCTION_PROPERTY
{
	gctINT					propertyType;
	gctSIZE_T				propertySize;
}
gcsKERNEL_FUNCTION_PROPERTY,
* gcsKERNEL_FUNCTION_PROPERTY_PTR;

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_KERNEL_FUNCTION_PROPERTY
{
	gctINT					propertyType;
	gctUINT32				propertySize;
}
* gcBINARY_KERNEL_FUNCTION_PROPERTY;

struct _gcsKERNEL_FUNCTION
{
	gcsOBJECT				object;

	gcSHADER				shader;
	gctSIZE_T				argumentArrayCount;
	gctSIZE_T				argumentCount;
	gcsFUNCTION_ARGUMENT_PTR		arguments;

	gctUINT16				label;

	/* Local address space size */
	gctSIZE_T				localMemorySize;

	/* Uniforms Args */
	gctSIZE_T				uniformArgumentArrayCount;
	gctSIZE_T				uniformArgumentCount;
	gcUNIFORM *				uniformArguments;
	gctINT					samplerIndex;

	/* Image-Sampler associations */
	gctSIZE_T				imageSamplerArrayCount;
	gctSIZE_T				imageSamplerCount;
	gcsIMAGE_SAMPLER_PTR			imageSamplers;

	/* Local variables. */
	gctSIZE_T				variableCount;
	gcVARIABLE *				variables;

	/* Kernel function properties */
	gctSIZE_T				propertyArrayCount;
	gctSIZE_T				propertyCount;
	gcsKERNEL_FUNCTION_PROPERTY_PTR		properties;
	gctSIZE_T				propertyValueArrayCount;
	gctSIZE_T				propertyValueCount;
	gctINT_PTR				propertyValues;

	gctUINT					codeStart;
	gctUINT					codeCount;
	gctUINT					codeEnd;

    gctBOOL					isMain;

	gctSIZE_T				nameLength;
	char					name[1];
};

/* Same structure, but inside a binary. */
typedef struct _gcBINARY_KERNEL_FUNCTION
{
	gctINT16				argumentCount;
	gctUINT16				label;
	gctUINT32				localMemorySize;
	gctINT16				uniformArgumentCount;
	gctINT16				samplerIndex;
	gctINT16				imageSamplerCount;
	gctINT16				variableCount;
	gctINT16				propertyCount;
	gctINT16				propertyValueCount;

	gctUINT16				codeStart;
	gctUINT16				codeCount;
	gctUINT16				codeEnd;

    gctUINT16				isMain;

	gctINT16				nameLength;
	char					name[1];
}
* gcBINARY_KERNEL_FUNCTION;

/* Index into current instruction. */
typedef enum _gcSHADER_INSTRUCTION_INDEX
{
	gcSHADER_OPCODE,
	gcSHADER_SOURCE0,
	gcSHADER_SOURCE1,
}
gcSHADER_INSTRUCTION_INDEX;

typedef struct _gcSHADER_LINK * gcSHADER_LINK;

/* Structure defining a linked references for a label. */
struct _gcSHADER_LINK
{
	gcSHADER_LINK				next;
	gctUINT						referenced;
};

typedef struct _gcSHADER_LABEL * gcSHADER_LABEL;

/* Structure defining a label. */
struct _gcSHADER_LABEL
{
	gcSHADER_LABEL				next;
	gctUINT					label;
	gctUINT					defined;
	gcSHADER_LINK				referenced;
};

/* The structure that defines the gcSHADER object to the outside world. */
struct _gcSHADER
{
	/* The base object. */
	gcsOBJECT				object;

	/* Frontend compiler version */
	gctUINT32				compilerVersion[2];

	/* Type of shader. */
	gctINT					type;

	/* Maximum of kernel function arguments, used to calculation the starting uniform index */
	gctUINT32				maxKernelFunctionArgs;

	/* Constant memory address space size for openCL */
	gctSIZE_T				constantMemorySize;
	gctCHAR	*				constantMemoryBuffer;

	/* Private memory address space size for openCL */
	gctSIZE_T				privateMemorySize;

	/* Local memory address space size for openCL, inherited from chosen kernel function */
	gctSIZE_T				localMemorySize;

	/* Attributes. */
	gctSIZE_T				attributeArrayCount;
	gctSIZE_T				attributeCount;
	gcATTRIBUTE *				attributes;

	/* Uniforms. */
	gctSIZE_T				uniformArrayCount;
	gctSIZE_T				uniformCount;
	gcUNIFORM *				uniforms;
	gctINT					samplerIndex;

	/* Outputs. */
	gctSIZE_T				outputArrayCount;
	gctSIZE_T				outputCount;
	gcOUTPUT *				outputs;

	/* Global variables. */
	gctSIZE_T				variableArrayCount;
	gctSIZE_T				variableCount;
	gcVARIABLE *				variables;

	/* Functions. */
	gctSIZE_T				functionArrayCount;
	gctSIZE_T				functionCount;
	gcFUNCTION *				functions;
	gcFUNCTION				currentFunction;

	/* Kernel Functions. */
	gctSIZE_T				kernelFunctionArrayCount;
	gctSIZE_T				kernelFunctionCount;
	gcKERNEL_FUNCTION *			kernelFunctions;
	gcKERNEL_FUNCTION			currentKernelFunction;

	/* Code. */
	gctSIZE_T				codeCount;
	gctUINT					lastInstruction;
	gcSHADER_INSTRUCTION_INDEX		instrIndex;
	gcSHADER_LABEL				labels;
	gcSL_INSTRUCTION			code;

	/* Load users for LOAD SW workaround optimization. */
	gctINT *				loadUsers;

#if GC_ENABLE_LOADTIME_OPT
	/* load-time optimization uniforms */
    gctINT                  ltcUniformCount;          /* load-time constant uniform count */
    gctUINT                 ltcUniformBegin;          /* the begin offset of ltc in uniforms */
    gctINT *                ltcCodeUniformIndex;      /* an array to map code index to uniform index,
                                                         element which has 0 value means no uniform for the code*/
    gctUINT                 ltcInstructionCount;      /* the total instruction count of the LTC expressions */
    gcSL_INSTRUCTION        ltcExpressions;           /* the expression array for ltc uniforms, which is a list of instructions */
#endif /* GC_ENABLE_LOADTIME_OPT */

	/* Optimization option. */
	gctUINT					optimizationOption;
};

/******************************************************************************\
|************************* gcSL_BRANCH_LIST structure. ************************|
\******************************************************************************/

typedef struct _gcSL_BRANCH_LIST * gcSL_BRANCH_LIST;

struct _gcSL_BRANCH_LIST
{
	/* Pointer to next gcSL_BRANCH_LIST structure in list. */
	gcSL_BRANCH_LIST	next;

	/* Pointer to generated instruction. */
	gctUINT				ip;

	/* Target instruction for branch. */
	gctUINT				target;

	/* Flag whether this is a branch or a call. */
	gctBOOL				call;
};

/******************************************************************************\
|**************************** gcLINKTREE structure. ***************************|
\******************************************************************************/

typedef struct _gcsLINKTREE_LIST *	gcsLINKTREE_LIST_PTR;

/* Structure that defines the linked list of dependencies. */
typedef struct _gcsLINKTREE_LIST
{
	/* Pointer to next dependent register. */
	gcsLINKTREE_LIST_PTR			next;

	/* Type of dependent register. */
	gcSL_TYPE						type;

	/* Index of dependent register. */
	gctINT							index;

	/* Reference counter. */
	gctINT							counter;
}
gcsLINKTREE_LIST;

/* Structure that defines the dependencies for an attribute. */
typedef struct _gcLINKTREE_ATTRIBUTE
{
	/* In-use flag. */
	gctBOOL						inUse;

	/* Instruction location the attribute was last used. */
	gctINT						lastUse;

	/* A linked list of all temporary registers using this attribute. */
	gcsLINKTREE_LIST_PTR		users;
}
* gcLINKTREE_ATTRIBUTE;

/* Structure that defines the dependencies for a temporary register. */
typedef struct _gcLINKTREE_TEMP
{
	/* In-use flag. */
	gctBOOL						inUse;

	/* Usage flags for the temporary register. */
	gctUINT8					usage;

	/* True if the reghister is used as an index. */
	gctBOOL						isIndex;

    /* True if the reghister is indexing. */
    gctBOOL                     isIndexing;

	/* Instruction locations that defines the temporary register. */
	gcsLINKTREE_LIST_PTR		defined;

	/* Instruction location the temporary register was last used. */
	gctINT						lastUse;

	/* Dependencies for the temporary register. */
	gcsLINKTREE_LIST_PTR		dependencies;

	/* Whether the register holds a constant. */
	gctINT8						constUsage[4];
	gctFLOAT					constValue[4];

	/* A linked list of all registers using this temporary register. */
	gcsLINKTREE_LIST_PTR		users;

	/* Physical register this temporary register is assigned to. */
	gctINT						assigned;
	gctUINT8					swizzle;
	gctINT						shift;

	/* Function arguments. */
	gctPOINTER					owner;
	gctBOOL						isOwnerKernel;
	gceINPUT_OUTPUT				inputOrOutput;

	/* Variable in shader's symbol table. */
	gcVARIABLE					variable;

    /* Format. */
    gcSL_FORMAT                 format;
}
* gcLINKTREE_TEMP;

/* Structure that defines the outputs. */
typedef struct _gcLINKTREE_OUTPUT
{
	/* In-use flag. */
	gctBOOL						inUse;

	/* Temporary register holding the output value. */
	gctINT						tempHolding;

	/* Fragment attribute linked to this vertex output. */
	gctINT						fragmentAttribute;
	gctINT						fragmentIndex;
	gctINT						fragmentIndexEnd;
}
* gcLINKTREE_OUTPUT;

typedef struct _gcsCODE_CALLER	*gcsCODE_CALLER_PTR;
typedef struct _gcsCODE_CALLER
{
	gcsCODE_CALLER_PTR			next;

	gctINT						caller;
}
gcsCODE_CALLER;

typedef struct _gcsCODE_HINT
{
	/* Pointer to function or kernel function to which this code belongs. */
	gctPOINTER					owner;

	/* Is the owner a kernel function?. */
	gctBOOL						isOwnerKernel;

	/* Callers to this instruction. */
	gcsCODE_CALLER_PTR			callers;

	/* Nesting of call. */
	gctINT						callNest;

	/* Last use for temp register used in code gen. */
	gctINT						lastUseForTemp;

    /* Last user of LOAD instruction for special optimization for LOAD SW workaround. */
    gctINT                      lastLoadUser;
    gctINT                      loadDestIndex;
}
gcsCODE_HINT, *gcsCODE_HINT_PTR;

/* Structure that defines the entire life and dependency for a shader. */
typedef struct _gcLINKTREE
{
	/* Pointer to the gcSHADER object. */
	gcSHADER					shader;

	/* Number of attributes. */
	gctSIZE_T					attributeCount;

	/* Attributes. */
	gcLINKTREE_ATTRIBUTE		attributeArray;

	/* Number of temporary registers. */
	gctSIZE_T					tempCount;

	/* Temporary registers. */
	gcLINKTREE_TEMP				tempArray;

	/* Number of outputs. */
	gctSIZE_T					outputCount;

	/* Outputs. */
	gcLINKTREE_OUTPUT			outputArray;

	/* Uniform usage. */
	gctSIZE_T					uniformUsage;

	/* Resource allocation passed. */
	gctBOOL						physical;

	/* Branch list. */
	gcSL_BRANCH_LIST			branch;

	/* Code hints. */
	gcsCODE_HINT_PTR			hints;
}
* gcLINKTREE;

/* Generate hardware states. */
gceSTATUS
gcLINKTREE_GenerateStates(
	IN gcLINKTREE Tree,
	IN gceSHADER_FLAGS Flags,
	IN OUT gctSIZE_T * StateBufferSize,
	IN OUT gctPOINTER * StateBuffer,
	OUT gcsHINT_PTR * Hints
	);

typedef struct _gcsCODE_GENERATOR	gcsCODE_GENERATOR;
typedef struct _gcsCODE_GENERATOR	* gcsCODE_GENERATOR_PTR;

gctUINT
gcsCODE_GENERATOR_GetIP(
	gcsCODE_GENERATOR_PTR CodeGen
	);

typedef gctBOOL (*gctSL_FUNCTION_PTR)(
	IN gcLINKTREE Tree,
	IN gcsCODE_GENERATOR_PTR CodeGen,
	IN gcSL_INSTRUCTION Instruction,
	IN OUT gctUINT32 * States
	);

typedef struct _gcsSL_PATTERN		gcsSL_PATTERN;
typedef struct _gcsSL_PATTERN		* gcsSL_PATTERN_PTR;
struct _gcsSL_PATTERN
{
	/* Positive: search index, aproaching zero.
	   Negative: code generation index aproaching zero. */
	gctINT							count;

	/* Opcode. */
	gctUINT							opcode;

	/* Destination reference number. */
	gctINT8							dest;

	/* Source 0 reference number. */
	gctINT8							source0;

	/* Source 1 reference number. */
	gctINT8							source1;

	/* Source 2 reference number. */
	gctINT8							source2;

	/* Sampler reference number. */
	gctINT8							sampler;

	/* Code generation function. */
	gctSL_FUNCTION_PTR				function;
};

gctSIZE_T
gcSHADER_GetHintSize(
	void
	);

gctBOOL
gcSHADER_CheckBugFixes10(
	void
	);

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_user_compiler_h_ */
